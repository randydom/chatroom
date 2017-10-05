#include <protocol/read_buffer.hpp>

using namespace std;

namespace protocol {
    const char* InvalidReadException::what() const noexcept {
        return "Invalid read attempted";
    }

    const char* SocketClosedException::what() const noexcept {
        return "Peer performed shutdown";
    }
}
