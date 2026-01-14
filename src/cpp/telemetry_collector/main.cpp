#include "shared_memory.h"
#include "metrics_collector.h"
#include "telemetry_generated.h"
#include "websocket_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <vector>
#include <sys/types.h>
#include <unistd.h>

volatile bool running = true;

void signal_handler(int signal) {
    running = false;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: telemetry_collector <job_id> <target_pid> <websocket_url>" << std::endl;
        return 1;
    }

    std::string job_id = argv[1];
    pid_t target_pid = std::stoi(argv[2]);
    std::string websocket_url = argv[3];

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        SharedMemoryReader shm_reader(job_id);
        MetricsCollector metrics_collector(target_pid);
        metrics_collector.initialize();

        WebSocketClient ws_client(websocket_url);
        if (!ws_client.connect()) {
            std::cerr << "Failed to connect to WebSocket" << std::endl;
            return 1;
        }

        auto next_tick = std::chrono::steady_clock::now();
        const auto interval = std::chrono::milliseconds(100);

        while (running) {
            next_tick += interval;

            auto snapshot = shm_reader.read_consistent();

            ProcessMetrics metrics = metrics_collector.collect();

            flatbuffers::FlatBufferBuilder builder(256);

            auto packet = Telemetry::CreateTelemetryPacket(
                builder,
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count(),
                snapshot.hands_processed,
                metrics.cpu_percent,
                metrics.memory_rss_kb,
                metrics.memory_vms_kb,
                metrics.thread_count,
                metrics.cpu_cycles,
                snapshot.status
            );

            builder.Finish(packet);

            std::vector<uint8_t> data(
                builder.GetBufferPointer(),
                builder.GetBufferPointer() + builder.GetSize()
            );
            ws_client.send_binary(data);

            if (snapshot.status == 1 || snapshot.status == 2 || kill(target_pid, 0) != 0) {
                break;
            }

            std::this_thread::sleep_until(next_tick);
        }

        metrics_collector.cleanup();
        ws_client.close();
        shm_reader.cleanup();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
