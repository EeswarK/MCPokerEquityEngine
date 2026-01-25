#ifndef API_SERVER_H
#define API_SERVER_H

#include "httplib.h"
#include "job_manager.h"
#include "engine/equity_engine.h"
#include <memory>

class APIServer {
private:
    std::unique_ptr<httplib::Server> server_;
    JobManager job_manager_;
    int port_;

    void setup_cors();
    void setup_routes();
    void execute_job(const std::string& job_id, const poker_engine::JobRequest& request);

public:
    explicit APIServer(int port = 8002);
    void run();
};

#endif // API_SERVER_H
