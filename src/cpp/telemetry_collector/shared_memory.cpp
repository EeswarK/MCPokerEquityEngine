#include "shared_memory.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include <cerrno>
#include <thread>
#include <chrono>

SharedMemoryReader::SharedMemoryReader(const std::string& job_id)
    : shm_fd(-1), data(nullptr), shm_name("/poker_telemetry_" + job_id) {
    std::string shm_path = "/dev/shm" + shm_name;
    shm_fd = open(shm_path.c_str(), O_RDONLY);
    if (shm_fd < 0) {
        throw std::runtime_error("Failed to open shared memory file: " + shm_path + " (errno: " + std::to_string(errno) + ")");
    }

    data = static_cast<CompleteSharedMemory*>(
        mmap(nullptr, sizeof(CompleteSharedMemory), PROT_READ, MAP_SHARED, shm_fd, 0)
    );

    if (data == MAP_FAILED) {
        close(shm_fd);
        throw std::runtime_error("Failed to map shared memory (errno: " + std::to_string(errno) + ")");
    }
}

SharedMemoryReader::~SharedMemoryReader() {
    cleanup();
}

TelemetrySnapshot SharedMemoryReader::read_telemetry_consistent() const {
    if (data == nullptr || data == MAP_FAILED) {
        throw std::runtime_error("Shared memory is invalid");
    }

    TelemetrySnapshot snapshot;
    uint32_t seq1, seq2;
    int retries = 0;
    const int max_retries = 1000;

    do {
        if (retries++ > max_retries) {
            throw std::runtime_error("Sequence lock read timeout");
        }
        seq1 = data->telemetry.seq.load(std::memory_order_acquire);

        if (seq1 & 1) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            continue;
        }

        snapshot.hands_processed = data->telemetry.hands_processed;
        snapshot.last_update_ns = data->telemetry.last_update_ns;
        snapshot.status = data->telemetry.status;

        seq2 = data->telemetry.seq.load(std::memory_order_acquire);

    } while (seq1 != seq2);

    return snapshot;
}

EquityResultsSnapshot SharedMemoryReader::read_equity_consistent() const {
    if (data == nullptr || data == MAP_FAILED) {
        throw std::runtime_error("Shared memory is invalid");
    }

    EquityResultsSnapshot snapshot;
    uint32_t seq1, seq2;
    int retries = 0;
    const int max_retries = 1000;

    do {
        if (retries++ > max_retries) {
            throw std::runtime_error("Equity sequence lock read timeout");
        }

        // Read sequence counter (non-atomic in Python, so we read directly)
        seq1 = data->equity_results.seq;

        if (seq1 & 1) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            continue;
        }

        snapshot.results_count = data->equity_results.results_count;
        std::memcpy(snapshot.hand_names, data->equity_results.hand_names, sizeof(snapshot.hand_names));
        std::memcpy(snapshot.results, data->equity_results.results, sizeof(snapshot.results));

        seq2 = data->equity_results.seq;

    } while (seq1 != seq2);

    return snapshot;
}

bool SharedMemoryReader::is_valid() const {
    return data != nullptr && data != MAP_FAILED;
}

void SharedMemoryReader::cleanup() {
    if (data != MAP_FAILED && data != nullptr) {
        munmap(data, sizeof(CompleteSharedMemory));
        data = nullptr;
    }
    if (shm_fd >= 0) {
        close(shm_fd);
        shm_fd = -1;
    }

    if (!shm_name.empty()) {
        std::string shm_path = "/dev/shm" + shm_name;
        unlink(shm_path.c_str());
    }
}
