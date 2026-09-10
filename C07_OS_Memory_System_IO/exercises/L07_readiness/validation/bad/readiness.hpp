#pragma once
#include <algorithm>
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

inline std::expected<drain_result, std::error_code> drain(native_endpoint endpoint, std::size_t max_bytes) {
    drain_result result;
    std::array<char, 4> buffer{};
    const auto want = static_cast<int>(std::min(buffer.size(), max_bytes));
#ifdef _WIN32
    const int n = ::recv(endpoint, buffer.data(), want, 0);
    if (n > 0) result.bytes.append(buffer.data(), static_cast<std::size_t>(n));
    else if (n == 0) result.eof = true;
    else if (::WSAGetLastError() == WSAEWOULDBLOCK) result.would_block = true;
    else return std::unexpected(std::error_code{::WSAGetLastError(), std::system_category()});
#else
    const ssize_t n = ::recv(endpoint, buffer.data(), static_cast<std::size_t>(want), 0);
    if (n > 0) result.bytes.append(buffer.data(), static_cast<std::size_t>(n));
    else if (n == 0) result.eof = true;
    else if (errno == EAGAIN || errno == EWOULDBLOCK) result.would_block = true;
    else return std::unexpected(std::error_code{errno, std::generic_category()});
#endif
    return result;
}
} // namespace c07_l07
