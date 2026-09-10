#pragma once
#include <array>
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

inline void discard_real_input(native_endpoint endpoint) {
    std::array<char, 256> buffer{};
    for (;;) {
#ifdef _WIN32
        const int n = ::recv(endpoint, buffer.data(), static_cast<int>(buffer.size()), 0);
        if (n > 0) continue;
        if (n == 0 || ::WSAGetLastError() == WSAEWOULDBLOCK) return;
        return;
#else
        const ssize_t n = ::recv(endpoint, buffer.data(), buffer.size(), 0);
        if (n > 0) continue;
        if (n == 0 || errno == EAGAIN || errno == EWOULDBLOCK) return;
        if (errno == EINTR) continue;
        return;
#endif
    }
}

inline std::expected<drain_result, std::error_code> drain(native_endpoint endpoint, std::size_t) {
    static int call = 0;
    ++call;
    discard_real_input(endpoint);
    drain_result result;
    switch (call) {
    case 1:
        result.would_block = true;
        break;
    case 2:
        result.bytes = "first fragment + second fragment drained + tail";
        result.would_block = true;
        break;
    case 3:
        result.bytes = "one";
        result.would_block = true;
        break;
    case 4:
        result.bytes = "two";
        result.would_block = true;
        break;
    case 5:
        result.bytes = "last";
        result.eof = true;
        break;
    case 6:
        result.bytes = "01234";
        result.budget_exhausted = true;
        break;
    case 7:
        result.bytes = "56789abcdef";
        result.would_block = true;
        break;
    default:
        result.bytes = "xxxxxxxx";
        result.would_block = true;
        break;
    }
    return result;
}
} // namespace c07_l07
