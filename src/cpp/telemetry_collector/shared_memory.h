#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <cstdint>
#include <atomic>
#include <string>

// Shared memory structure for telemetry data
// Uses sequence lock pattern to prevent torn reads on 64-bit integers
// Python writes are NOT atomic, so we need a consistency check
struct alignas(64) TelemetrySharedMemory {
    // Sequence counter for consistency (incremented before and after write)
    // Reader checks: if odd, write in progress; if changed during read, retry
    std::atomic<uint32_t> seq;

    uint32_t _padding1;  // Align next field to 8-byte boundary

    // Atomic counter for hands processed (updated by evaluator)
    uint64_t hands_processed;

    // Timestamp of last update (nanoseconds since epoch)
    uint64_t last_update_ns;

    // Status flags (0 = running, 1 = completed, 2 = failed)
    uint8_t status;

    // Reserved for future use (padding to cache line size)
    uint8_t _reserved[39];
};

// Size must be exactly one cache line (64 bytes) for cache coherency
static_assert(sizeof(TelemetrySharedMemory) == 64, "Shared memory struct must be 64 bytes");
static_assert(alignof(TelemetrySharedMemory) == 64, "Shared memory must be 64-byte aligned");

struct TelemetrySnapshot {
    uint64_t hands_processed;
    uint64_t last_update_ns;
    uint8_t status;
};

class SharedMemoryReader {
private:
    int shm_fd;
    TelemetrySharedMemory* data;
    std::string shm_name;

public:
    SharedMemoryReader(const std::string& job_id);
    ~SharedMemoryReader();

    TelemetrySnapshot read_consistent() const;
    bool is_valid() const;
    void cleanup();
};

#endif // SHARED_MEMORY_H
