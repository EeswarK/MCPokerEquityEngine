# Telemetry Shared Memory Specification

## Structure

The shared memory segment contains a single `TelemetrySharedMemory` struct (64 bytes).

## Naming Convention

Shared memory segments are named: `/poker_telemetry_{job_id}`

Where `job_id` is the unique job identifier (e.g., UUID).

## Layout

- `seq` (uint32_t, atomic): Sequence counter for consistency (incremented before/after writes)
- `hands_processed` (uint64_t): Counter incremented by evaluator every n hands
- `last_update_ns` (uint64_t): Timestamp of last update in nanoseconds
- `status` (uint8_t): Status flag (0=running, 1=completed, 2=failed)
- `_reserved` (39 bytes): Reserved for future use

## Sequence Lock Pattern

**Why:** Python's 64-bit writes are NOT atomic on all systems. A 32-bit system (or misaligned access) may perform two 32-bit writes, causing torn reads.

**Writer (Python):**

1. Increment `seq` (odd = write in progress)
2. Write `hands_processed`, `last_update_ns`, `status`
3. Increment `seq` again (even = write complete)

**Reader (C++):**

1. Read `seq` → if odd, retry (write in progress)
2. Read `hands_processed`, `last_update_ns`, `status`
3. Read `seq` again → if changed, retry (write happened during read)

## Evaluator Responsibilities

1. Create shared memory segment with unique name
2. Initialize `seq = 0`, `hands_processed = 0`, `last_update_ns = current_time`, `status = 0`
3. Every n hands (e.g., 1000), use sequence lock to update:
   - `seq++` (make odd)
   - Write `hands_processed`, `last_update_ns`
   - `seq++` (make even)
4. On completion, set `status = 1` using sequence lock
5. On failure, set `status = 2` using sequence lock
6. **Do NOT clean up** shared memory (C++ collector handles it to survive crashes)

## Telemetry Collector Responsibilities

1. Open existing shared memory segment by name
2. Read `hands_processed` and `last_update_ns` every 100ms using sequence lock
3. Calculate rate: `(current_hands - previous_hands) / time_delta`
4. Monitor `status` for completion/failure
5. **Clean up shared memory** on job completion or when `target_pid` no longer exists (handles crashes)
