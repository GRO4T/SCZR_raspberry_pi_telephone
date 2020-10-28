#ifndef __ERROR_HPP__
#define __ERROR_HPP__

#include <stdexcept>
#include <cstring>
#include <errno.h>

class BackendException :public std::runtime_error {
public:
    BackendException(const std::string& msg)
        : std::runtime_error(msg) {}
    BackendException(const char* msg)
        : std::runtime_error(msg) {}
    BackendException()
        : std::runtime_error(strerror(errno)) {}
};

#endif

