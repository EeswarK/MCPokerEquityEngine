#include "shared_memory.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>

SharedMemoryReader::SharedMemoryReader(const std::string& job_id)
    : shm_fd(-1), data(nullptr), shm_name("/poker_telemetry_" + job_id) {
    shm_fd = shm_open(shm_name.c_str(), O_RDONLY, 0);
    if (shm_fd < 0) {
        throw std::runtime_error("Failed to open shared memory: " + shm_name);
    }

    data = static_cast<TelemetrySharedMemory*>(
        mmap(nullptr, sizeof(TelemetrySharedMemory), PROT_READ, MAP_SHARED, shm_fd, 0)
    );

    if (data == MAP_FAILED) {
        close(shm_fd);
        throw std::runtime_error("Failed to map shared memory");
    }
}

SharedMemoryReader::~SharedMemoryReader() {
    cleanup();
}

TelemetrySnapshot SharedMemoryReader::read_consistent() const {
    TelemetrySnapshot snapshot;
    uint32_t seq1, seq2;

    do {
        seq1 = data->seq.load(std::memory_order_acquire);

        if (seq1 & 1) {
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
    }
    if (shm_fd >= 0) {
        close(shm_fd);
    }

    if (!shm_name.empty()) {
        shm_unlink(shm_name.c_str());
    }
}
