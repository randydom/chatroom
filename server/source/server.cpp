#include <csignal>
#include <iostream>
#include <utility>

#include <sys/socket.h>

#include <server.hpp>
#include <socket/tcp_client_socket.hpp>

using namespace protocol;
using namespace std;

Server::Server(const string port) :
    chat_app(),
    server_socket(port, max_connections)
{
    signal(SIGPIPE, SIG_IGN);

    server_socket.set_non_blocking(true);

#ifdef DEBUG
    server_socket.set_reuse_address(true);
#endif

    server_socket.bind();
    server_socket.listen(max_pending_connections);
}

void Server::run() {
    cout << "Server initialized and running on port " << server_socket.get_port() << "." << endl;

    while (true) {
        server_socket.poll(50, [=](TCPClientSocket&& socket, const ConnectionID connection_id) {
            return Connection<State>(chat_app, forward<TCPClientSocket>(socket), connection_id);
        });
    }
}
