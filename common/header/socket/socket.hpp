#pragma once

class Socket {
private:
    bool non_blocking;
    bool reuse_address;

protected:
    int fd;

    Socket() noexcept;
    Socket(const int fd) noexcept;

    bool is_reuse_address() const noexcept;
    void set_reuse_address(const bool reuse_address);

public:
    Socket(const int domain, const int type, const int protocol);
    virtual ~Socket();
    Socket(Socket const &) = delete;
    Socket(Socket&&) noexcept;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) noexcept;

    int get_fd() const noexcept;
    bool is_non_blocking() const noexcept;
    void set_non_blocking(const bool non_blocking);
};
