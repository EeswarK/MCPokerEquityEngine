#include "job_manager.h"

std::shared_ptr<JobState> JobManager::create_job(const std::string& job_id) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto state = std::make_shared<JobState>(job_id);
    jobs_[job_id] = state;
    return state;
}

std::shared_ptr<JobState> JobManager::get_job(const std::string& job_id) const {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = jobs_.find(job_id);
    return (it != jobs_.end()) ? it->second : nullptr;
}

void JobManager::delete_job(const std::string& job_id) {
    std::lock_guard<std::mutex> guard(mutex_);
    jobs_.erase(job_id);
}
