#pragma once

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <exception.hpp>
#include <socket/socket.hpp>
#include <socket/tcp_client_socket.hpp>

using ConnectionID = std::size_t;

template <typename TConnection>
class PollData {
private:
    ConnectionID connection_sequence_number;
    std::size_t connection_fds_index;
    std::size_t max_connections;
    std::size_t number_of_connections;
    std::unordered_map<int, std::size_t> connection_fds_map;
    std::unordered_map<int, TConnection> connections_map;
    std::vector<pollfd> connection_fds;

    template <typename AddConnectionLambda>
    void add_connection(std::string address, std::string port, const int fd, AddConnectionLambda&& add_connection_lambda) {
        assert(number_of_connections < max_connections - 1);
        assert(connection_fds_map.find(fd) == connection_fds_map.cend());
        assert(connections_map.find(fd) == connections_map.cend());
        
        if (connection_fds_index == max_connections) {
            connection_fds_index = 1;

            for (size_t i{1}; i < max_connections; ++i) {
                const auto& poll_fd = connection_fds[i];

                if (poll_fd.fd >= 0) {
                    if (i != connection_fds_index) {
                        connection_fds[connection_fds_index] = poll_fd;
                        connection_fds_map[poll_fd.fd] = connection_fds_index;
                    }

                    ++connection_fds_index;
                }
            }
        }

        auto socket = TCPClientSocket(address, port, fd);
        socket.set_non_blocking(true);

        connection_fds[connection_fds_index] = { fd, POLLRDNORM, 0 };
        connection_fds_map.emplace(fd, connection_fds_index);
        connections_map.emplace(fd, std::forward<AddConnectionLambda>(add_connection_lambda)(std::move(socket), ++connection_sequence_number));
        ++connection_fds_index;
        ++number_of_connections;
    }

    void remove_connection(const int fd) {
        assert(number_of_connections > 0);
        assert(connection_fds_map.find(fd) != connection_fds_map.cend());
        assert(connections_map.find(fd) != connections_map.cend());

        --number_of_connections;

        const auto index = connection_fds_map[fd];
        connection_fds[index].fd = -1;

        connection_fds_map.erase(fd);
        connections_map.erase(fd);
    }

public:
    PollData(const std::size_t max_connections) :
        PollData{max_connections, -1}
    {

    }

    PollData(const std::size_t max_connections, const int listen_fd) :
        connection_sequence_number(0),
        connection_fds_index(1),
        max_connections(max_connections),
        number_of_connections(0),
        connection_fds_map(),
        connections_map(),
        connection_fds(max_connections)
    {
        assert(max_connections > 1);

        connection_fds_map.reserve(max_connections);
        connections_map.reserve(max_connections);
        connection_fds[0].events = POLLRDNORM;
        connection_fds[0].fd = listen_fd;

        for (size_t i{1}; i < max_connections; ++i) {
            connection_fds[i].fd = -1;
        }
    }

    int get_listen_fd() const noexcept {
        return connection_fds[0].fd;
    }

    template <typename AddConnectionLambda>
    void poll(const int timeout, AddConnectionLambda&& add_connection_lambda) {
        auto connections_ready = ::poll(connection_fds.data(),
                                        connection_fds_index,
                                        timeout);

        if (connections_ready == -1) {
            throw errno_to_system_error("Failed to poll socket");
        }

        if (connection_fds[0].revents & POLLRDNORM) {
            int fd;
            sockaddr address;
            socklen_t address_size = sizeof(sockaddr);

            do {
                if (number_of_connections < max_connections) {
                    fd = accept(connection_fds[0].fd, &address, &address_size);
                                
                    if (fd == -1) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            throw errno_to_system_error("Failed to accept connection");
                        }
                    } else if (number_of_connections < max_connections) {
                        std::string ip_address;
                        std::string port;

                        if (address.sa_family == AF_UNSPEC) {
                            switch (address_size) {
                                case sizeof(sockaddr_in):
                                    address.sa_family = AF_INET;
                                    break;
                                
                                case sizeof(sockaddr_in6):
                                    address.sa_family = AF_INET6;
                                    break;
                                
                                default:
                                    assert(false);
                            }
                        }

                        switch (address.sa_family) {
                            case AF_INET: {
                                sockaddr_in *address_in = reinterpret_cast<sockaddr_in*>(&address);
                                ip_address.resize(INET_ADDRSTRLEN);
                                inet_ntop(AF_INET, &(address_in->sin_addr),
                                            &ip_address[0],
                                            INET_ADDRSTRLEN);
                                port = std::to_string(ntohs(address_in->sin_port));
                                break;
                            }

                            case AF_INET6: {
                                sockaddr_in6 *address_in6 = reinterpret_cast<sockaddr_in6*>(&address);
                                ip_address.resize(INET6_ADDRSTRLEN);
                                inet_ntop(AF_INET6, &(address_in6->sin6_addr),
                                            &ip_address[0],
                                            INET6_ADDRSTRLEN);
                                port = std::to_string(ntohs(address_in6->sin6_port));
                                break;
                            }

                            default:
                                assert(false);
                        }

                        add_connection(ip_address,
                                        port,
                                        fd,
                                        std::forward<AddConnectionLambda>(add_connection_lambda));
                    }
                } else {
                    std::cerr << "Ignoring further connections due to maximum connections reached." << std::endl;
                    break;
                }
            } while (fd != -1);

            if (--connections_ready == 0) {
                return;
            }
        }

        for (std::size_t i{1}; i < connection_fds_index; ++i) {
            const auto fd = connection_fds[i].fd;

            if (fd >= 0) {
                auto& connection = connections_map.at(fd);

                if (connection_fds[i].revents > 0) {
                    try {
                        if (connection.handle_events(connection_fds[i].revents)) {
                            remove_connection(fd);
                            continue;
                        } 
                    } catch (const std::exception& e) {
                        std::cerr << "Connection (ID: " << connection.get_id() << ") removed due to error: " << e.what() << std::endl;
                        remove_connection(fd);
                        continue;
                    } catch (...) {
                        std::cerr << "Connection (ID: " << connection.get_id() << ") removed due to error." << std::endl;
                        remove_connection(fd);
                        continue;
                    }
                }

                if (connection.is_ready_to_write()) {
                    connection_fds[i].events = POLLRDNORM | POLLWRNORM;
                } else {
                    connection_fds[i].events = POLLRDNORM;
                }
            }
        }
    }

    void set_listen_fd(const int listen_fd) noexcept {
        connection_fds[0].fd = listen_fd;
    }
};

template <typename TConnection>
class TCPServerSocket : public Socket {
private:
    addrinfo* server_addresses;
    PollData<TConnection> poll_data;
    const std::string port;

public:
    TCPServerSocket(std::string port, const std::size_t max_connections) :
        Socket(),
        server_addresses(nullptr),
        poll_data(max_connections),
        port(port)
    {
        addrinfo hints = {};
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_socktype = SOCK_STREAM;
    
        const auto result = getaddrinfo(nullptr, port.c_str(), &hints, &server_addresses);
    
        if (result != 0) {
            throw AddrInfoException(result, "Failed to get address information");
        }
    
        fd = socket(server_addresses->ai_family,
                    server_addresses->ai_socktype,
                    server_addresses->ai_protocol);
    
        if (fd == -1) {
            freeaddrinfo(server_addresses);
            throw errno_to_system_error("Failed to create socket");
        }

        poll_data.set_listen_fd(fd);
    }

    virtual ~TCPServerSocket() {
        if (server_addresses != nullptr) {
            freeaddrinfo(server_addresses);
        }
    }

    TCPServerSocket(TCPServerSocket const &) = delete;
    TCPServerSocket(TCPServerSocket&&) = default;
    TCPServerSocket& operator=(const TCPServerSocket&) = delete;
    TCPServerSocket& operator=(TCPServerSocket&&) = default;

    using Socket::is_reuse_address;
    using Socket::set_reuse_address;

    void bind() const {
        if (::bind(fd, server_addresses->ai_addr, server_addresses->ai_addrlen) == -1) {
            throw errno_to_system_error("Failed to bind address to socket");
        }
    }

    std::string get_port() const {
        return port;
    }

    void listen(const std::size_t max_pending_connections) const {
        if (::listen(fd, max_pending_connections) == -1) {
            throw errno_to_system_error("Failed to set socket as listening");
        }
    }

    template <typename AddConnectionLambda>
    void poll(const int timeout, AddConnectionLambda&& add_connection_lambda) {
        poll_data.poll(timeout, std::forward<AddConnectionLambda>(add_connection_lambda));
    }
};
