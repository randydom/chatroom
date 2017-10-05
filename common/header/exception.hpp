#pragma once

#include <exception>
#include <string>
#include <system_error>

class AddrInfoException: public std::exception {
protected:
    const int error_code;
    const std::string what_arg;

public:
    AddrInfoException(const int error_code, std::string what_arg) noexcept;
    ~AddrInfoException() = default;

    int get_error_code() const noexcept;
    const char* what() const noexcept override;
};

std::system_error errno_to_system_error(const char* const what_arg);