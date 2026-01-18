#ifndef API_JOB_MANAGER_H
#define API_JOB_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include "engine/equity_result.h"
#include <memory>

enum class JobStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED
};

struct JobState {
    std::string job_id;
    JobStatus status;
    double progress;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point completed_at;
    std::string error;
    std::unordered_map<std::string, EquityResult> results;
    std::unordered_map<std::string, double> current_results;

    mutable std::mutex lock;

    JobState(const std::string& id)
        : job_id(id), status(JobStatus::PENDING), progress(0.0),
          created_at(std::chrono::system_clock::now()) {}

    void start() {
        std::lock_guard<std::mutex> guard(lock);
        status = JobStatus::RUNNING;
    }

    void complete(const std::unordered_map<std::string, EquityResult>& res) {
        std::lock_guard<std::mutex> guard(lock);
        status = JobStatus::COMPLETED;
        results = res;
        completed_at = std::chrono::system_clock::now();
        progress = 1.0;
    }

    void fail(const std::string& err) {
        std::lock_guard<std::mutex> guard(lock);
        status = JobStatus::FAILED;
        error = err;
        completed_at = std::chrono::system_clock::now();
    }

    void update_progress(double prog, const std::unordered_map<std::string, double>& curr) {
        std::lock_guard<std::mutex> guard(lock);
        progress = prog;
        current_results = curr;
    }
};

class JobManager {
private:
    std::unordered_map<std::string, std::shared_ptr<JobState>> jobs_;
    mutable std::mutex mutex_;

public:
    std::shared_ptr<JobState> create_job(const std::string& job_id);
    std::shared_ptr<JobState> get_job(const std::string& job_id) const;
    void delete_job(const std::string& job_id);
};

#endif // API_JOB_MANAGER_H
