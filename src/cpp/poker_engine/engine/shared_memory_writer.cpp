#include "shared_memory_writer.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <algorithm>
#include <iostream>

SharedMemoryWriter::SharedMemoryWriter(const std::string& job_id)
    : job_id_(job_id),
      shm_name_("/poker_telemetry_" + job_id),
      shm_fd_(-1),
      shm_ptr_(nullptr),
      data_(nullptr) {}

SharedMemoryWriter::~SharedMemoryWriter() {
    close();
}

// Matches: src/python/utils/shared_memory.py:61-86
bool SharedMemoryWriter::create() {
    // On Linux, shared memory is typically in /dev/shm
    // But we use shm_open which handles the namespace correctly.
    shm_fd_ = shm_open(shm_name_.c_str(), O_CREAT | O_RDWR | O_EXCL, 0600);

    if (shm_fd_ < 0) {
        std::cerr << "shm_open failed for " << shm_name_ << ": " << strerror(errno) << std::endl;
        return false;
    }

    // Set size (Python line 66)
    if (ftruncate(shm_fd_, sizeof(CompleteSharedMemory)) < 0) {
        ::close(shm_fd_);
        shm_fd_ = -1;
        return false;
    }

    // Memory map (Python lines 68-70)
    shm_ptr_ = mmap(nullptr, sizeof(CompleteSharedMemory),
                   PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);

    if (shm_ptr_ == MAP_FAILED) {
        ::close(shm_fd_);
        shm_fd_ = -1;
        return false;
    }

    data_ = static_cast<CompleteSharedMemory*>(shm_ptr_);

    // Initialize telemetry fields (Python lines 73-79)
    data_->telemetry.seq.store(0, std::memory_order_release);

    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    data_->telemetry.job_start_ns = now_ns;
    data_->telemetry.hands_processed = 0;
    data_->telemetry.last_update_ns = now_ns;
    data_->telemetry.status = 0;  // Running

    // Initialize equity results (Python lines 78-79)
    data_->equity_results.seq = 0;
    data_->equity_results.results_count = 0;

    return true;
}

// Matches: src/python/utils/shared_memory.py:88-98
void SharedMemoryWriter::update_hands(uint64_t count) {
    if (!data_) return;

    // Sequence lock pattern (Python lines 93, 98)
    data_->telemetry.seq.fetch_add(1, std::memory_order_release);  // Make odd

    data_->telemetry.hands_processed = count;
    data_->telemetry.last_update_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    data_->telemetry.seq.fetch_add(1, std::memory_order_release);  // Make even
}

// Matches: src/python/utils/shared_memory.py:100-107
void SharedMemoryWriter::set_status(uint8_t status) {
    if (!data_) return;

    data_->telemetry.seq.fetch_add(1, std::memory_order_release);
    data_->telemetry.status = status;
    data_->telemetry.seq.fetch_add(1, std::memory_order_release);
}

// Matches: src/python/utils/shared_memory.py:109-157
void SharedMemoryWriter::update_equity_results(
    const std::unordered_map<std::string, EquityResult>& results) {

    if (!data_) return;

    // Sequence lock (Python line 118)
    data_->equity_results.seq++;  // Non-atomic like Python

    // Update results count (Python line 121)
    data_->equity_results.results_count = std::min(
        static_cast<uint32_t>(results.size()),
        static_cast<uint32_t>(MAX_HANDS)
    );

    // Write each result (Python lines 124-155)
    size_t idx = 0;
    for (const auto& pair : results) {
        const std::string& hand_name = pair.first;
        const EquityResult& result = pair.second;
        
        if (idx >= MAX_HANDS) break;

        // Write hand name (Python lines 128-134)
        std::memset(data_->equity_results.hand_names[idx], 0, 8);
        std::strncpy(data_->equity_results.hand_names[idx], hand_name.c_str(), 7);

        // Write equity data (Python lines 137-141)
        data_->equity_results.results[idx].equity = result.equity;
        data_->equity_results.results[idx].wins = result.wins;
        data_->equity_results.results[idx].ties = result.ties;
        data_->equity_results.results[idx].losses = result.losses;
        data_->equity_results.results[idx].simulations = result.total_simulations;

        // Write win-method matrix (Python lines 144-148)
        for (int our_type = 0; our_type < 10; ++our_type) {
            for (int opp_type = 0; opp_type < 10; ++opp_type) {
                data_->equity_results.results[idx].win_method_matrix[our_type][opp_type] =
                    result.win_method_matrix[our_type][opp_type];
            }
        }

        // Write loss-method matrix (Python lines 151-155)
        for (int opp_type = 0; opp_type < 10; ++opp_type) {
            for (int our_type = 0; our_type < 10; ++our_type) {
                data_->equity_results.results[idx].loss_method_matrix[opp_type][our_type] =
                    result.loss_method_matrix[opp_type][our_type];
            }
        }

        idx++;
    }

    data_->equity_results.seq++;  // Complete sequence lock
}

// Matches: src/python/utils/shared_memory.py:159-164
void SharedMemoryWriter::close() {
    if (shm_ptr_ && shm_ptr_ != MAP_FAILED) {
        munmap(shm_ptr_, sizeof(CompleteSharedMemory));
        shm_ptr_ = nullptr;
    }
    if (shm_fd_ >= 0) {
        ::close(shm_fd_);
        shm_fd_ = -1;
    }
    // DO NOT unlink - C++ collector handles cleanup
}
