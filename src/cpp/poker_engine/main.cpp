#include "api/server.h"
#include <iostream>
#include <signal.h>

volatile bool running = true;

void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "C++ Poker Equity Engine v0.1.0" << std::endl;

    int port = 8002;  // Default port
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::cout << "Starting API server on port " << port << "..." << std::endl;

    try {
        APIServer server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}