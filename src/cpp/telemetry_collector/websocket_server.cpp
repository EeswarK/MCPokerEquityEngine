#include "websocket_server.h"
#include <iostream>

WebSocketServer::WebSocketServer(int port, const std::string& job_id) 
    : port(port), job_id(job_id), running(false) {
    server = std::make_unique<ix::WebSocketServer>(port, "0.0.0.0");
    
    server->setOnConnectionCallback([this](std::weak_ptr<ix::WebSocket> ws_weak, 
                                           std::shared_ptr<ix::ConnectionState> connectionState) {
        auto ws = ws_weak.lock();
        if (!ws) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(connections_mutex);
        connections.insert(ws);
        std::cerr << "Client connected for job: " << this->job_id << " (total: " << connections.size() << ")" << std::endl;
        
        ws->setOnMessageCallback([this, ws](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Error) {
                std::cerr << "WebSocket error on " << this->job_id << ": " << msg->errorInfo.reason << std::endl;
            } else if (msg->type == ix::WebSocketMessageType::Close) {
                std::lock_guard<std::mutex> lock(connections_mutex);
                connections.erase(ws);
                std::cerr << "Client disconnected for job: " << this->job_id << std::endl;
            }
        });
    });
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
    auto res = server->listen();
    if (!res.first) {
        std::cerr << "Failed to start WebSocket server on port " << port << ": " << res.second << std::endl;
        return false;
    }
    
    server->start();
    running = true;
    std::cerr << "WebSocket server started on port " << port << std::endl;
    return true;
}

void WebSocketServer::stop() {
    if (running) {
        server->stop();
        running = false;
        std::cerr << "WebSocket server stopped" << std::endl;
    }
}

void WebSocketServer::broadcast_binary(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(connections_mutex);
    
    auto conn_it = connections.begin();
    while (conn_it != connections.end()) {
        auto ws = *conn_it;
        if (ws->getReadyState() == ix::ReadyState::Open) {
            ws->sendBinary(ix::IXWebSocketSendData(
                reinterpret_cast<const char*>(data.data()),
                data.size()
            ));
            ++conn_it;
        } else {
            conn_it = connections.erase(conn_it);
        }
    }
}
