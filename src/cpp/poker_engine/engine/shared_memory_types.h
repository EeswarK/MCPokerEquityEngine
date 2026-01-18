#ifndef ENGINE_SHARED_MEMORY_TYPES_H
#define ENGINE_SHARED_MEMORY_TYPES_H

#include <cstdint>
#include <atomic>

#define MAX_HANDS 169

// EXACT copy of telemetry collector structs - do not modify
struct HandEquityResult {
    double equity;
    uint32_t wins;
    uint32_t ties;
    uint32_t losses;
    uint32_t simulations;
    uint32_t win_method_matrix[10][10];
    uint32_t loss_method_matrix[10][10];
    uint32_t _padding[2];
};
static_assert(sizeof(HandEquityResult) == 832, "HandEquityResult must be 832 bytes");

struct EquityResultsSegment {
    uint32_t seq;
    uint32_t results_count;
    char hand_names[MAX_HANDS][8];
    HandEquityResult results[MAX_HANDS];
};

struct alignas(64) TelemetrySharedMemory {
    std::atomic<uint32_t> seq;
    uint32_t _padding1;
    uint64_t job_start_ns;
    uint64_t hands_processed;
    uint64_t last_update_ns;
    uint8_t status;
    uint8_t _reserved[31];
};
static_assert(sizeof(TelemetrySharedMemory) == 64, "Must be 64 bytes");
static_assert(alignof(TelemetrySharedMemory) == 64, "Must be 64-byte aligned");

struct CompleteSharedMemory {
    TelemetrySharedMemory telemetry;
    EquityResultsSegment equity_results;
};

#endif // ENGINE_SHARED_MEMORY_TYPES_H
