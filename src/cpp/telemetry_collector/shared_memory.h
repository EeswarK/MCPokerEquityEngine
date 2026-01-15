#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <cstdint>
#include <atomic>
#include <string>
#include <cstring>

// Maximum number of poker hands in 13x13 matrix (pairs + suited + offsuit)
#define MAX_HANDS 169

// Per-hand equity result
struct HandEquityResult {
    double equity;        // Win rate (0.0 to 1.0)
    uint32_t wins;        // Number of wins
    uint32_t ties;        // Number of ties
    uint32_t losses;      // Number of losses
    uint32_t simulations; // Total simulations for this hand
    uint32_t win_method_matrix[10][10];  // Win frequency by hand type: [our_type][opp_type]
    uint32_t _padding[6]; // Padding to make total 448 bytes (8 + 4 + 4 + 4 + 4 + 400 + 24 = 448)
};

static_assert(sizeof(HandEquityResult) == 448, "HandEquityResult must be 448 bytes");

// Equity results segment (follows telemetry struct in shared memory)
struct EquityResultsSegment {
    uint32_t seq;              // Sequence lock for results updates
    uint32_t results_count;     // Number of valid results in array
    char hand_names[MAX_HANDS][8];  // Hand names (e.g., "AA", "AKs", "72o")
    HandEquityResult results[MAX_HANDS];  // Results array
};

// Shared memory structure for telemetry data
// Uses sequence lock pattern to prevent torn reads on 64-bit integers
// Python writes are NOT atomic, so we need a consistency check
struct alignas(64) TelemetrySharedMemory {
    // Sequence counter for consistency (incremented before and after write)
    // Reader checks: if odd, write in progress; if changed during read, retry
    std::atomic<uint32_t> seq;

    uint32_t _padding1;  // Align next field to 8-byte boundary

    // Job start timestamp (nanoseconds since epoch)
    uint64_t job_start_ns;

    // Atomic counter for hands processed (updated by evaluator)
    uint64_t hands_processed;

    // Timestamp of last update (nanoseconds since epoch)
    uint64_t last_update_ns;

    // Status flags (0 = running, 1 = completed, 2 = failed)
    uint8_t status;

    // Reserved for future use (padding to cache line size)
    uint8_t _reserved[31];
};

// Size must be exactly one cache line (64 bytes) for cache coherency
static_assert(sizeof(TelemetrySharedMemory) == 64, "Shared memory struct must be 64 bytes");
static_assert(alignof(TelemetrySharedMemory) == 64, "Shared memory must be 64-byte aligned");

// Complete shared memory layout
struct CompleteSharedMemory {
    TelemetrySharedMemory telemetry;
    EquityResultsSegment equity_results;
};

struct TelemetrySnapshot {
    uint64_t job_start_ns;
    uint64_t hands_processed;
    uint64_t last_update_ns;
    uint8_t status;
};

struct EquityResultsSnapshot {
    uint32_t results_count;
    char hand_names[MAX_HANDS][8];
    HandEquityResult results[MAX_HANDS];
};

class SharedMemoryReader {
private:
    int shm_fd;
    CompleteSharedMemory* data;
    std::string shm_name;

public:
    SharedMemoryReader(const std::string& job_id);
    ~SharedMemoryReader();

    TelemetrySnapshot read_telemetry_consistent() const;
    EquityResultsSnapshot read_equity_consistent() const;
    bool is_valid() const;
    void cleanup();
};

#endif // SHARED_MEMORY_H
