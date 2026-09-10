#pragma once
#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

namespace c07_l02 {
inline std::expected<std::uintmax_t, std::error_code> copy_file_exact(
    const std::filesystem::path& source, const std::filesystem::path& target, std::size_t chunk_size) {
    std::ifstream in(source, std::ios::binary);
    std::ofstream out(target, std::ios::binary | std::ios::trunc);
    std::vector<char> buffer(chunk_size == 0 ? 1 : chunk_size);
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const auto n = in.gcount();
    out.write(buffer.data(), n);
    return static_cast<std::uintmax_t>(n);
}
} // namespace c07_l02
