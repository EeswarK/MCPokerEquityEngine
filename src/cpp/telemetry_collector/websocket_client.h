#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include <vector>

class WebSocketClient {
private:
    ix::WebSocket ws;
    bool connected;

public:
    WebSocketClient(const std::string& url);
    bool connect();
    void send_binary(const std::vector<uint8_t>& data);
    void close();
    bool is_connected() const { return connected; }
};

#endif
