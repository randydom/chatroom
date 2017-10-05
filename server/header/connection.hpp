#pragma once

#include <cstddef>
#include <utility>

#include <chat_app.hpp>
#include <socket/tcp_client_socket.hpp>
#include <socket/tcp_server_socket.hpp>

template <typename State>
class Connection {
private:
    const ConnectionID id;
    State state;
    TCPClientSocket socket;

public:
    Connection(ChatApp& chat_app, TCPClientSocket socket, ConnectionID id) :
        id(id),
        state(chat_app),
        socket(std::move(socket))
    {

    }

    Connection(Connection const &) = delete;
    Connection(Connection&&) = default;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = default;

    ConnectionID get_id() const noexcept {
        return id;
    }

    bool handle_events(const short events)
    {
        if (events & POLLERR) {
            return true;
        }
    
        bool should_close = false;
    
        if (events & POLLRDNORM) {
            should_close = state.read(socket);
        } else if (events & POLLHUP) {
            return true;
        }
    
        if (events & POLLWRNORM) {
            should_close = should_close || state.write(socket);
        }
    
        return should_close;
    }

    bool is_ready_to_write() const noexcept {
        return state.is_ready_to_write();
    }
};
