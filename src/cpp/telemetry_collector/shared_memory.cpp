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

    data = static_cast<TelemetrySharedMemory*>(
        mmap(nullptr, sizeof(TelemetrySharedMemory), PROT_READ, MAP_SHARED, shm_fd, 0)
    );

    if (data == MAP_FAILED) {
        close(shm_fd);
        throw std::runtime_error("Failed to map shared memory (errno: " + std::to_string(errno) + ")");
    }
}

SharedMemoryReader::~SharedMemoryReader() {
    cleanup();
}

TelemetrySnapshot SharedMemoryReader::read_consistent() const {
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
        seq1 = data->seq.load(std::memory_order_acquire);

        if (seq1 & 1) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            continue;
        }

        snapshot.hands_processed = data->hands_processed;
        snapshot.last_update_ns = data->last_update_ns;
        snapshot.status = data->status;

        seq2 = data->seq.load(std::memory_order_acquire);

    } while (seq1 != seq2);

    return snapshot;
}

bool SharedMemoryReader::is_valid() const {
    return data != nullptr && data != MAP_FAILED;
}

void SharedMemoryReader::cleanup() {
    if (data != MAP_FAILED && data != nullptr) {
        munmap(data, sizeof(TelemetrySharedMemory));
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
