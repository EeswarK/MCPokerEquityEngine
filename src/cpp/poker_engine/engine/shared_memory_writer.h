#ifndef ENGINE_SHARED_MEMORY_WRITER_H
#define ENGINE_SHARED_MEMORY_WRITER_H

#include "shared_memory_types.h"
#include "equity_result.h"
#include <string>
#include <unordered_map>

class SharedMemoryWriter {
private:
    std::string job_id_;
    std::string shm_name_;
    int shm_fd_;
    void* shm_ptr_;
    CompleteSharedMemory* data_;

public:
    explicit SharedMemoryWriter(const std::string& job_id);
    ~SharedMemoryWriter();

    // Create shared memory segment (Python: create())
    bool create();

    // Update hands processed counter (Python: update_hands())
    void update_hands(uint64_t count);

    // Set completion status (Python: set_status())
    void set_status(uint8_t status);

    // Update equity results (Python: update_equity_results())
    void update_equity_results(const std::unordered_map<std::string, EquityResult>& results);

    // Close shared memory (Python: close())
    void close();
};

#endif // ENGINE_SHARED_MEMORY_WRITER_H
