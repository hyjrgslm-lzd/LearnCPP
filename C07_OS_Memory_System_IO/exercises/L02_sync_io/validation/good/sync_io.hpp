#pragma once
#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <limits>
#include <system_error>
#include <vector>

namespace c07_l02 {
inline std::expected<std::uintmax_t, std::error_code> copy_file_exact(
    const std::filesystem::path& source, const std::filesystem::path& target, std::size_t chunk_size) {
    if (chunk_size == 0) return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    const auto stream_limit = static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max());
    if (chunk_size > stream_limit) return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    std::ifstream in(source, std::ios::binary);
    if (!in) return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
    std::ofstream out(target, std::ios::binary | std::ios::noreplace);
    if (!out) return std::unexpected(std::make_error_code(std::errc::file_exists));
    std::vector<char> buffer(chunk_size);
    std::uintmax_t copied = 0;
    while (in) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto n = in.gcount();
        if (n <= 0) break;
        out.write(buffer.data(), n);
        if (!out) return std::unexpected(std::make_error_code(std::errc::io_error));
        copied += static_cast<std::uintmax_t>(n);
    }
    if (!in.eof()) return std::unexpected(std::make_error_code(std::errc::io_error));
    out.flush();
    if (!out) return std::unexpected(std::make_error_code(std::errc::io_error));
    out.close();
    if (!out) return std::unexpected(std::make_error_code(std::errc::io_error));
    return copied;
}
} // namespace c07_l02
