#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <ixwebsocket/IXWebSocketServer.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <set>

class WebSocketServer {
private:
    std::unique_ptr<ix::WebSocketServer> server;
    std::set<std::shared_ptr<ix::WebSocket>> connections;
    std::mutex connections_mutex;
    std::string job_id;
    int port;
    bool running;

public:
    WebSocketServer(int port, const std::string& job_id);
    ~WebSocketServer();
    
    bool start();
    void stop();
    void broadcast_binary(const std::vector<uint8_t>& data);
    bool is_running() const { return running; }
    int get_port() const { return port; }
};

#endif
