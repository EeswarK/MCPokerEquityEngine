#include "metrics_collector.h"
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/resource.h>
#include <chrono>

MetricsCollector::MetricsCollector(pid_t pid)
    : target_pid(pid), perf_fd(-1), last_utime(0), last_stime(0), last_timestamp_ns(0) {}

bool MetricsCollector::initialize() {
    perf_fd = -1;
    return true;
}

ProcessMetrics MetricsCollector::collect() {
    ProcessMetrics metrics = {};
    metrics.cpu_cycles = 0;

    std::string stat_path = "/proc/" + std::to_string(target_pid) + "/stat";
    std::ifstream stat_file(stat_path);
    if (stat_file.is_open()) {
        std::string line;
        std::getline(stat_file, line);
        std::istringstream iss(line);

        std::string token;
        uint64_t utime = 0;
        uint64_t stime = 0;
        uint64_t rss_pages = 0;

        for (int i = 0; i < 22 && iss >> token; ++i) {
            if (i == 13) {
                utime = std::stoull(token);
            } else if (i == 14) {
                stime = std::stoull(token);
            } else if (i == 21) {
                rss_pages = std::stoull(token);
            }
        }

        metrics.memory_rss_kb = rss_pages * 4;

        auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        if (last_timestamp_ns > 0) {
            uint64_t total_time = utime + stime;
            uint64_t last_total_time = last_utime + last_stime;
            uint64_t time_diff = total_time - last_total_time;
            uint64_t ns_diff = now_ns - last_timestamp_ns;
            double seconds_diff = ns_diff / 1e9;

            if (seconds_diff > 0) {
                long clock_ticks_per_sec = sysconf(_SC_CLK_TCK);
                double cpu_time_diff = static_cast<double>(time_diff) / clock_ticks_per_sec;
                metrics.cpu_percent = (cpu_time_diff / seconds_diff) * 100.0;
            } else {
                metrics.cpu_percent = 0.0;
            }
        } else {
            metrics.cpu_percent = 0.0;
        }

        last_utime = utime;
        last_stime = stime;
        last_timestamp_ns = now_ns;
    }

    std::string status_path = "/proc/" + std::to_string(target_pid) + "/status";
    std::ifstream status_file(status_path);
    if (status_file.is_open()) {
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.find("VmSize:") == 0) {
                std::istringstream iss(line);
                std::string label, value, unit;
                iss >> label >> value >> unit;
                if (unit == "kB") {
                    metrics.memory_vms_kb = std::stoull(value);
                }
            } else if (line.find("Threads:") == 0) {
                std::istringstream iss(line);
                std::string label, value;
                iss >> label >> value;
                metrics.thread_count = std::stoul(value);
            }
        }
    }

    return metrics;
}

MetricsCollector::~MetricsCollector() {
    cleanup();
}

void MetricsCollector::cleanup() {
    if (perf_fd >= 0) {
        close(perf_fd);
        perf_fd = -1;
    }
}
