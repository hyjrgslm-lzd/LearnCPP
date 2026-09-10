#pragma once
#include <expected>
#include <string>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
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

inline std::expected<drain_result, std::error_code> drain(native_endpoint, std::size_t) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}
} // namespace c07_l07
