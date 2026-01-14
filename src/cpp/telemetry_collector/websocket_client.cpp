#include "websocket_client.h"
#include <iostream>
#include <thread>
#include <chrono>

WebSocketClient::WebSocketClient(const std::string& url) : connected(false) {
    ws.setUrl(url);

    ws.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Error) {
            std::cerr << "WebSocket error: " << msg->errorInfo.reason << std::endl;
        }
    });
}

bool WebSocketClient::connect() {
    ws.start();

    for (int i = 0; i < 50 && !connected; ++i) {
        if (ws.getReadyState() == ix::ReadyState::Open) {
            connected = true;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return connected;
}

void WebSocketClient::send_binary(const std::vector<uint8_t>& data) {
    if (connected && ws.getReadyState() == ix::ReadyState::Open) {
        ws.sendBinary(ix::IXWebSocketSendData(
            reinterpret_cast<const char*>(data.data()),
            data.size()
        ));
    }
}

void WebSocketClient::close() {
    ws.stop();
    connected = false;
}
