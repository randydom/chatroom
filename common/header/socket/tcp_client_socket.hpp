#pragma once

#include <cstddef>
#include <string>

#include <socket/socket.hpp>

using namespace std;

class TCPClientSocket : public Socket {
protected:
    template <typename TConnection>
    friend class PollData;

    const std::string address;
    const std::string port;

    TCPClientSocket(std::string address, std::string port, const int fd) noexcept;

public:
    TCPClientSocket(std::string address, std::string port);
    virtual ~TCPClientSocket() = default;
    TCPClientSocket(TCPClientSocket const &) = delete;
    TCPClientSocket(TCPClientSocket&&) = default;
    TCPClientSocket& operator=(const TCPClientSocket&) = delete;
    TCPClientSocket& operator=(TCPClientSocket&&) = default;

    bool recv(unsigned char* const buffer, std::size_t& size);
    void send(const unsigned char* const buffer, std::size_t& size);
};