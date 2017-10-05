#pragma once

#include <string>

#include <chat_app.hpp>
#include <connection.hpp>
#include <protocol/state.hpp>
#include <socket/tcp_server_socket.hpp>

class Server {
private:
    static constexpr std::size_t max_connections = 64;
    static constexpr int max_pending_connections = 16;

    ChatApp chat_app;
    TCPServerSocket<Connection<protocol::State>> server_socket;

public:
    Server(const std::string port);
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run();
};
