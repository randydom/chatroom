#include <cerrno>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <exception.hpp>

using namespace std;

AddrInfoException::AddrInfoException(const int error_code, string what_arg) noexcept :
    error_code(error_code),
    what_arg(what_arg + ": " + gai_strerror(error_code))
{
    
}

int AddrInfoException::get_error_code() const noexcept {
    return error_code;
}

const char* AddrInfoException::what() const noexcept {
    return what_arg.c_str();
}

system_error errno_to_system_error(const char* const what_arg) {
    return system_error(errno, system_category(), what_arg);
}