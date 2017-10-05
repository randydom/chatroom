#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <exception>

#include <arpa/inet.h>

#include <protocol/message.hpp>
#include <socket/tcp_client_socket.hpp>

namespace protocol {
    class InvalidReadException : public std::exception {
    public:
        const char* what() const noexcept override;
    };

    class SocketClosedException : public std::exception {
    public:
        const char* what() const noexcept override;
    };

    template <std::size_t BufferSize>
    class ReadBuffer {
    private:
        std::array<unsigned char, BufferSize> buffer;
        std::size_t bytes_processed;
        std::size_t bytes_read;
        std::size_t bytes_remaining;

    public:
        bool is_ready() const noexcept {
            return bytes_remaining == 0;
        }

        void read_from_socket(TCPClientSocket& socket) {
            if (is_ready()) {
                return;
            }

            const auto original_remaining = bytes_remaining;
            auto helper = [=]() {
                bytes_read += original_remaining - bytes_remaining;
            };

            try {
                if (socket.recv(buffer.data() + bytes_read, bytes_remaining)) {
                    throw SocketClosedException();
                }
            } catch (...) {
                helper();
                throw;
            }

            helper();
        }

        unsigned char read_u8() {
            unsigned char u8;
            
            if (!try_read_u8(u8)) {
                throw InvalidReadException();
            }
            
            return u8;
        }

        unsigned short read_u16() {
            unsigned short u16;

            if (!try_read_u16(u16)) {
                throw InvalidReadException();
            }
            
            return u16;
        }

        void reset(const std::size_t size) noexcept {
            bytes_processed = 0;
            bytes_read = 0;
            bytes_remaining = size;
        }

        bool try_read_u8(unsigned char& u8) {
            if (bytes_processed + 1 > bytes_read) {
                return false;
            }

            u8 = buffer[bytes_processed];
            ++bytes_processed;
            return true;
        }

        bool try_read_u16(unsigned short& u16) {
            if (bytes_processed + 2 > bytes_read) {
                return false;
            }

            u16 = ntohs(buffer[bytes_processed + 1] << 8 | buffer[bytes_processed]);
            bytes_processed += 2;
            return true;
        }
    };
}
