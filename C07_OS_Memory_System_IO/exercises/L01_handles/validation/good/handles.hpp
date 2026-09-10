#pragma once
#include <c07/os.hpp>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <system_error>
#include <vector>

namespace c07_l01 {
inline std::expected<std::size_t, std::error_code> create_note(
    const std::filesystem::path& path, std::span<const char> payload) {
    auto file = c07::create_new_file(path);
    if (!file) return std::unexpected(file.error());
    std::vector<std::byte> bytes;
    bytes.reserve(payload.size());
    for (char ch : payload) bytes.push_back(static_cast<std::byte>(ch));
    if (auto write = c07::write_all(file->get(), bytes); !write) return std::unexpected(write.error());
    return bytes.size();
}
} // namespace c07_l01
