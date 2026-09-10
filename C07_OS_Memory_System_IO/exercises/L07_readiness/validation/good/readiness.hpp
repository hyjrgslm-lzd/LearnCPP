#pragma once
#include <expected>
#include <string>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#else
#include <cerrno>
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace c07_l07 {
#ifdef _WIN32
using native_endpoint = SOCKET;
#else
using native_endpoint = int;
#endif

struct drain_result {
    std::string bytes;
    bool would_block = false;
    bool eof = false;
    bool budget_exhausted = false;
};

inline std::expected<drain_result, std::error_code> drain(native_endpoint endpoint, std::size_t max_bytes) {
    drain_result result;
    char ch = 0;
    while (result.bytes.size() != max_bytes) {
#ifdef _WIN32
        const int n = ::recv(endpoint, &ch, 1, 0);
        if (n == 1) result.bytes.push_back(ch);
        else if (n == 0) {
            result.eof = true;
            return result;
        } else {
            const int error = ::WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                result.would_block = true;
                return result;
            }
            return std::unexpected(std::error_code{error, std::system_category()});
        }
#else
        const ssize_t n = ::recv(endpoint, &ch, 1, 0);
        if (n == 1) result.bytes.push_back(ch);
        else if (n == 0) {
            result.eof = true;
            return result;
        } else {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                result.would_block = true;
                return result;
            }
            return std::unexpected(std::error_code{errno, std::generic_category()});
        }
#endif
    }
    result.budget_exhausted = true;
    return result;
}
} // namespace c07_l07
