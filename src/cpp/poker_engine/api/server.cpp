#include "server.h"
#include "json_utils.h"
#include <thread>
#include <sstream>
#include <random>
#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

// Simple UUID v4 generator
std::string generate_uuid() {
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    thread_local std::uniform_int_distribution<> dis(0, 15);
    thread_local std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

APIServer::APIServer(int port)
    : server_(std::make_unique<httplib::Server>()), port_(port) {
    setup_cors();
    setup_routes();
}

// Helper function to add CORS headers to a response
void add_cors_headers(const httplib::Request& req, httplib::Response& res) {
    std::string origin = req.get_header_value("Origin");
    res.set_header("Access-Control-Allow-Origin", origin.empty() ? "*" : origin);
    res.set_header("Access-Control-Allow-Credentials", "true");
}

void APIServer::setup_cors() {
    // CORS preflight handler - runs for ALL OPTIONS requests
    server_->Options(".*", [](const httplib::Request& req, httplib::Response& res) {
        std::string origin = req.get_header_value("Origin");
        std::cout << "CORS: Handling OPTIONS from origin: " << (origin.empty() ? "*" : origin) << std::endl;

        res.status = 204;
        res.set_header("Access-Control-Allow-Origin", origin.empty() ? "*" : origin);
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With, Accept, Origin, Cache-Control");
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.set_header("Access-Control-Max-Age", "86400");
    });
}

void APIServer::setup_routes() {
    // POST /api/jobs - Create new equity calculation job
    server_->Post("/api/jobs", [this](const httplib::Request& req, httplib::Response& res) {
        std::cout << "Received POST /api/jobs" << std::endl;

        // Parse request
        std::unordered_map<std::string, std::vector<Card>> range_spec;
        std::vector<Card> board;
        int num_opponents, num_simulations, num_workers;
        std::string mode, algorithm;
        std::vector<std::string> optimizations;

        if (!parse_create_job_request(req.body, range_spec, board,
                                      num_opponents, num_simulations, mode, 
                                      algorithm, optimizations, num_workers)) {
            std::cerr << "Failed to parse request body" << std::endl;
            res.status = 400;
            res.set_content(serialize_error_response("Invalid request body"), "application/json");
            add_cors_headers(req, res);
            return;
        }

        // Generate job ID
        std::string job_id = generate_uuid();
        std::cout << "Created job: " << job_id << " mode=" << mode << " algorithm=" << algorithm << std::endl;

        auto job_state = job_manager_.create_job(job_id);

        // Build telemetry WebSocket URL (match Python logic)
        const char* host = std::getenv("TELEMETRY_HOST");
        const char* protocol_env = std::getenv("TELEMETRY_WS_PROTOCOL");

        std::string ws_host = host ? host : "localhost";
        std::string ws_protocol = protocol_env ? protocol_env : "ws";

        std::stringstream telemetry_url;
        if (ws_protocol == "wss") {
            // WSS: Don't include port (proxied through nginx on 443)
            telemetry_url << ws_protocol << "://" << ws_host << "/telemetry/" << job_id;
        } else {
            // WS: Include port for local development
            const char* port_env = std::getenv("TELEMETRY_PORT");
            int ws_port = port_env ? std::stoi(port_env) : 8001;
            telemetry_url << ws_protocol << "://" << ws_host << ":" << ws_port << "/telemetry/" << job_id;
        }

        // Create job request
        poker_engine::JobRequest job_req;
        job_req.range_spec = range_spec;
        job_req.board = board;
        job_req.num_opponents = num_opponents;
        job_req.num_simulations = num_simulations;
        job_req.mode = mode;
        job_req.algorithm = algorithm;
        job_req.optimizations = optimizations;
        job_req.num_workers = num_workers;

        // Execute job in background thread
        std::thread([this, job_id, job_req]() {
            this->execute_job(job_id, job_req);
        }).detach();

        // Return response
        auto time_t = std::chrono::system_clock::to_time_t(job_state->created_at);
        std::stringstream created_at_ss;
        created_at_ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");

        res.status = 201;
        res.set_content(
            serialize_create_job_response(job_id, "pending", created_at_ss.str(), telemetry_url.str()),
            "application/json"
        );
        add_cors_headers(req, res);
    });

    // GET /api/jobs/{job_id}/status - Get job status
    server_->Get(R"(/api/jobs/([^/]+)/status)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string job_id = req.matches[1];

        auto job_state = job_manager_.get_job(job_id);
        if (!job_state) {
            res.status = 404;
            res.set_content(serialize_error_response("Job not found"), "application/json");
            add_cors_headers(req, res);
            return;
        }

        res.status = 200;
        res.set_content(serialize_job_status_response(*job_state), "application/json");
        add_cors_headers(req, res);
    });

    // GET /health - Health check
    server_->Get("/health", [](const httplib::Request& req, httplib::Response& res) {
        res.status = 200;
        res.set_content(serialize_health_response(), "application/json");
        add_cors_headers(req, res);
    });
}

void APIServer::execute_job(const std::string& job_id, const poker_engine::JobRequest& request) {
    auto job_state = job_manager_.get_job(job_id);
    if (!job_state) return;

    pid_t telemetry_pid = -1;

    try {
        job_state->start();

        // Spawn telemetry collector process
        const char* telemetry_binary = std::getenv("TELEMETRY_COLLECTOR_BINARY");
        if (!telemetry_binary) {
            telemetry_binary = "../telemetry_collector/build/telemetry_collector";
        }

        const char* telemetry_port_env = std::getenv("TELEMETRY_PORT");
        std::string telemetry_port = telemetry_port_env ? telemetry_port_env : "8001";

        // Fork to spawn telemetry collector
        telemetry_pid = fork();
        if (telemetry_pid == 0) {
            // Child process - execute telemetry collector
            pid_t parent_pid = getppid();
            std::string parent_pid_str = std::to_string(parent_pid);

            execl(telemetry_binary, telemetry_binary,
                  job_id.c_str(),
                  parent_pid_str.c_str(),
                  telemetry_port.c_str(),
                  nullptr);

            // If execl returns, it failed
            std::cerr << "Failed to execute telemetry collector: " << telemetry_binary << std::endl;
            _exit(1);
        } else if (telemetry_pid > 0) {
            std::cout << "Spawned telemetry collector for job " << job_id
                      << " (PID: " << telemetry_pid << ")" << std::endl;
        } else {
            std::cerr << "Failed to fork telemetry collector process" << std::endl;
        }

        // Create engine with job_id for shared memory
        // Note: algorithm and optimizations are passed through request and used in calculate_range_equity
        poker_engine::EquityEngine engine(request.algorithm.empty() ? request.mode : request.algorithm, job_id);

        // Set progress callback
        engine.set_progress_callback(
            [job_state](double progress, const std::unordered_map<std::string, double>& results) {
                job_state->update_progress(progress, results);
            }
        );

        // Run calculation
        auto results = engine.calculate_range_equity(request);

        job_state->complete(results);

    } catch (const std::exception& e) {
        job_state->fail(e.what());
    }

    // Clean up telemetry process
    if (telemetry_pid > 0) {
        kill(telemetry_pid, SIGTERM);
        int status;
        // Wait up to 2 seconds for process to exit
        for (int i = 0; i < 20; i++) {
            if (waitpid(telemetry_pid, &status, WNOHANG) > 0) {
                break;
            }
            usleep(100000); // 100ms
        }
        // Force kill if still alive
        kill(telemetry_pid, SIGKILL);
        waitpid(telemetry_pid, &status, 0);
    }
}

void APIServer::run() {
    std::cout << "Starting API server on port " << port_ << "..." << std::endl;
    std::cout << "Server listening at http://0.0.0.0:" << port_ << std::endl;
    server_->listen("0.0.0.0", port_);
}
