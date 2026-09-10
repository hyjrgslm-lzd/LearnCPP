#pragma once
#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <span>
#include <system_error>

namespace c07_l01 {
inline std::expected<std::size_t, std::error_code> create_note(
    const std::filesystem::path& path, std::span<const char> payload) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return std::unexpected(std::make_error_code(std::errc::io_error));
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    return payload.size();
}
} // namespace c07_l01
