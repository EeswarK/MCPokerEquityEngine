#include "shared_memory.h"
#include "metrics_collector.h"
#include "telemetry_generated.h"
#include "websocket_server.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>

volatile bool running = true;

void signal_handler(int signal) {
    running = false;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: telemetry_collector <job_id> <target_pid> [port]" << std::endl;
        return 1;
    }

    std::string job_id = argv[1];
    pid_t target_pid = std::stoi(argv[2]);
    int port = (argc >= 4) ? std::stoi(argv[3]) : 8001;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        std::cerr << "Opening shared memory for job: " << job_id << std::endl;
        SharedMemoryReader shm_reader(job_id);
        std::cerr << "Shared memory opened successfully" << std::endl;

        MetricsCollector metrics_collector(target_pid);
        metrics_collector.initialize();

        std::cerr << "Starting WebSocket server on port " << port << std::endl;
        WebSocketServer ws_server(port, job_id);
        if (!ws_server.start()) {
            std::cerr << "Failed to start WebSocket server" << std::endl;
            return 1;
        }
        std::cerr << "WebSocket server ready. Clients can connect to: ws://localhost:" << port << "/telemetry/" << job_id << std::endl;

        auto next_tick = std::chrono::steady_clock::now();
        const auto interval = std::chrono::milliseconds(100);
        int packet_count = 0;

        while (running) {
            next_tick += interval;

            try {
                auto telemetry_snapshot = shm_reader.read_telemetry_consistent();
                auto equity_snapshot = shm_reader.read_equity_consistent();

                ProcessMetrics metrics = metrics_collector.collect();

                flatbuffers::FlatBufferBuilder builder(8192);  // Larger buffer for equity results

                // Build equity results vector
                std::vector<flatbuffers::Offset<Telemetry::HandEquity>> equity_results_vec;
                for (uint32_t i = 0; i < equity_snapshot.results_count && i < MAX_HANDS; i++) {
                    auto hand_name = builder.CreateString(equity_snapshot.hand_names[i]);
                    auto hand_equity = Telemetry::CreateHandEquity(
                        builder,
                        hand_name,
                        equity_snapshot.results[i].equity,
                        equity_snapshot.results[i].wins,
                        equity_snapshot.results[i].ties,
                        equity_snapshot.results[i].losses,
                        equity_snapshot.results[i].simulations
                    );
                    equity_results_vec.push_back(hand_equity);
                }

                auto equity_results_fb = builder.CreateVector(equity_results_vec);

                auto packet = Telemetry::CreateTelemetryPacket(
                    builder,
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count(),
                    telemetry_snapshot.job_start_ns,
                    telemetry_snapshot.hands_processed,
                    metrics.cpu_percent,
                    metrics.memory_rss_kb,
                    metrics.memory_vms_kb,
                    metrics.thread_count,
                    metrics.cpu_cycles,
                    telemetry_snapshot.status,
                    equity_results_fb
                );

                builder.Finish(packet);

                std::vector<uint8_t> data(
                    builder.GetBufferPointer(),
                    builder.GetBufferPointer() + builder.GetSize()
                );
                ws_server.broadcast_binary(data);

                if (telemetry_snapshot.status == 1 || telemetry_snapshot.status == 2 || kill(target_pid, 0) != 0) {
                    std::cerr << "Job finished or process died. Status: " << static_cast<int>(telemetry_snapshot.status) << std::endl;
                    break;
                }

                packet_count++;
                if (packet_count % 10 == 0) {
                    std::cerr << "Packets sent: " << packet_count
                              << ", hands: " << telemetry_snapshot.hands_processed
                              << ", equity results: " << equity_snapshot.results_count << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error reading shared memory: " << e.what() << std::endl;
                break;
            }

            std::this_thread::sleep_until(next_tick);
        }

        std::cerr << "Shutting down. Total packets: " << packet_count << std::endl;
        metrics_collector.cleanup();
        ws_server.stop();
        shm_reader.cleanup();
        std::cerr << "Cleanup complete" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
