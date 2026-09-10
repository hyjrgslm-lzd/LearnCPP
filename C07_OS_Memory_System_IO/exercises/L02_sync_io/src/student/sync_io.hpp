#pragma once
#include <cstddef>
#include <expected>
#include <filesystem>
#include <system_error>

namespace c07_l02 {
std::expected<std::uintmax_t, std::error_code> copy_file_exact(
    const std::filesystem::path&, const std::filesystem::path&, std::size_t) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}
} // namespace c07_l02
