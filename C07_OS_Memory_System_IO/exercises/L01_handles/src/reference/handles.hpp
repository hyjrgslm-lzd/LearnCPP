#pragma once
#include <c07/os.hpp>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <system_error>

namespace c07_l01 {
inline std::expected<std::size_t, std::error_code> create_note(
    const std::filesystem::path& path, std::span<const char> payload) {
    auto file = c07::create_new_file(path);
    if (!file) return std::unexpected(file.error());
    const auto written = c07::write_all(file->get(), c07::bytes_of(payload));
    if (!written) return std::unexpected(written.error());
    return payload.size();
}
} // namespace c07_l01
