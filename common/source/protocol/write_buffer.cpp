#include <protocol/write_buffer.hpp>

using namespace std;

namespace protocol {
    WriteBufferFullException::WriteBufferFullException(const size_t bytes_written) noexcept :
        bytes_written(bytes_written)
    {

    }

    size_t WriteBufferFullException::get_bytes_written() const noexcept {
        return bytes_written;
    }

    const char* WriteBufferFullException::what() const noexcept {
        return "Write buffer is full.";
    }
}
