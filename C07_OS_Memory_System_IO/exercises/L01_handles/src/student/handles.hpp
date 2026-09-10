#pragma once
#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <system_error>

namespace c07_l01 {
std::expected<std::size_t, std::error_code> create_note(
    const std::filesystem::path&, std::span<const char>) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}
} // namespace c07_l01
