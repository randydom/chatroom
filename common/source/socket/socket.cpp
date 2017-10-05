#include <cassert>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <exception.hpp>
#include <socket/socket.hpp>

using namespace std;

Socket::Socket() noexcept :
    non_blocking(false),
    reuse_address(false),
    fd(-1)
{

}

Socket::Socket(const int fd) noexcept : 
    non_blocking(false),
    fd(fd)
{
    assert(fd >= 0);
}

Socket::Socket(const int domain, const int type, const int protocol) :
    non_blocking(false),
    fd(socket(domain, type, protocol))
{
    if (fd == -1) {
        throw errno_to_system_error("Failed to create socket");
    }
}

Socket::Socket(Socket&& other) noexcept :
    non_blocking(other.non_blocking),
    reuse_address(other.reuse_address),
    fd(other.fd)
{
    other.fd = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    non_blocking = other.non_blocking;
    reuse_address = other.reuse_address;
    fd = other.fd;

    other.fd = -1;

    return *this;
}

Socket::~Socket() {
    if (fd >= 0) {
        // TODO: Handle close error
        
        close(fd);
    }
}

int Socket::get_fd() const noexcept {
    return fd;
}

bool Socket::is_non_blocking() const noexcept {
    return non_blocking;
}

bool Socket::is_reuse_address() const noexcept {
    return reuse_address;
}

void Socket::set_non_blocking(const bool non_blocking) {
    if (this->non_blocking == non_blocking) {
        return;
    }

    auto flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        throw errno_to_system_error("Failed to get status flags for socket");
    }

    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    if (fcntl(fd, F_SETFL, flags) == -1) {
        throw errno_to_system_error("Failed to set non-blocking flag for socket");
    }

    this->non_blocking = non_blocking;
}

void Socket::set_reuse_address(const bool reuse_address) {
    auto value = static_cast<int>(reuse_address);

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) == -1) {
        throw errno_to_system_error("Failed to set reuse address for socket");
    }

    this->reuse_address = reuse_address;
}
