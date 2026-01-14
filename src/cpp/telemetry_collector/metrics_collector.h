#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H

#include <cstdint>
#include <string>
#include <sys/types.h>

struct ProcessMetrics {
    double cpu_percent;
    uint64_t memory_rss_kb;
    uint64_t memory_vms_kb;
    uint32_t thread_count;
    uint64_t cpu_cycles;
};

class MetricsCollector {
private:
    pid_t target_pid;
    int perf_fd;
    uint64_t last_utime;
    uint64_t last_stime;
    uint64_t last_timestamp_ns;

public:
    MetricsCollector(pid_t pid);
    ~MetricsCollector();

    bool initialize();
    ProcessMetrics collect();
    void cleanup();
};

#endif
