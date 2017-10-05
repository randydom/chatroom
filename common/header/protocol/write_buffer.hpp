#pragma once

#include <array>
#include <cstddef>
#include <exception>

#include <arpa/inet.h>

#include <socket/tcp_client_socket.hpp>
#include <iostream>
namespace protocol {
    class WriteBufferFullException : public std::exception {
    private:
        const std::size_t bytes_written;

    public:
        WriteBufferFullException(const std::size_t write_count) noexcept;

        std::size_t get_bytes_written() const noexcept;
        const char* what() const noexcept override;
    };

    template <std::size_t BufferSize>
    class WriteBuffer {
    private:
        std::array<unsigned char, BufferSize> buffer;
        std::size_t buffer_head;
        std::size_t buffer_tail;
        std::size_t bytes_written;

    public:
        bool is_empty() const noexcept {
            return buffer_head == buffer_tail;
        }

        bool is_full() const noexcept {
            return (buffer_head + 1) % BufferSize == buffer_tail;
        }

        void write_to_socket(TCPClientSocket& socket) {
            if (is_empty()) {
                return;
            }

            auto size = buffer_head > buffer_tail ?
                        buffer_head - buffer_tail :
                        BufferSize - buffer_tail;
            const auto original_size = size;
            auto helper = [&]() {
                auto bytes_written = original_size - size;
                buffer_tail += bytes_written;
            
                if (buffer_tail == BufferSize) {
                    buffer_tail = 0;
                }
            };
    
            try {
                socket.send(buffer.data() + buffer_tail, size);
            } catch (...) {
                helper();
                throw;
            }
    
            helper();
        }

        void write_u8(const unsigned char u8) {
            if (is_full()) {
                const auto bytes_written = this->bytes_written;
                this->bytes_written = 0;
                throw WriteBufferFullException(bytes_written);
            }
    
            buffer[buffer_head] = u8;
            buffer_head = (buffer_head + 1) % BufferSize;
        }

        void write_u16(unsigned short u16) {
            u16 = htons(u16);
    
            bytes_written = 0;
            write_u8(u16 & 0xFF);
    
            ++bytes_written;
            write_u8(u16 >> 8);
        }
    };
}
