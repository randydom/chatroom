#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <exception.hpp>
#include <socket/tcp_client_socket.hpp>

using namespace std;

TCPClientSocket::TCPClientSocket(string address, string port, const int fd) noexcept :
    Socket(fd),
    address(address),
    port(port)
{
    
}

TCPClientSocket::TCPClientSocket(string address, string port) :
    Socket(),
    address(address),
    port(port)
{
    addrinfo* addresses;
    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const auto result = getaddrinfo(address.c_str(), port.c_str(), &hints, &addresses);

    if (result != 0) {
        throw AddrInfoException(result, "Failed to get address information");
    }

    fd = socket(addresses->ai_family,
                addresses->ai_socktype,
                addresses->ai_protocol);

    if (fd == -1) {
        freeaddrinfo(addresses);
        throw errno_to_system_error("Failed to create socket");
    }

    if (connect(fd, addresses->ai_addr, addresses->ai_addrlen) == -1) {
        freeaddrinfo(addresses);
        throw errno_to_system_error("Failed to connect to address");
    }
}

bool TCPClientSocket::recv(unsigned char* const buffer, size_t& size) {
    if (size == 0) {
        return false;
    }

    const auto total_size = size;

    while (size > 0) {
        auto bytes_received = ::recv(fd, buffer + total_size - size, size, 0);

        if (bytes_received == -1) {
            throw errno_to_system_error("Failed to receive data from socket");
        } else if (bytes_received == 0) {
            return true;
        } else {
            size -= bytes_received;
        }
    }

    return false;
}

void TCPClientSocket::send(const unsigned char* const buffer, size_t& size) {
    size_t total_size = size;

    while (size > 0) {
        auto bytes_written = ::send(fd, buffer + total_size - size, size, 0);

        if (bytes_written == -1) {
            throw errno_to_system_error("Failed to write data to socket");
        } else {
            size -= bytes_written;
        }
    }
}
