#ifndef C07_L04_MAPPING_LAB_HPP
#define C07_L04_MAPPING_LAB_HPP

#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <system_error>

#include <c07/memory.hpp>

namespace c07_l04 {

struct window_plan {
    std::uint64_t aligned_offset{};
    std::size_t delta{};
    std::size_t mapped_length{};
    std::size_t visible_length{};
};

enum class write_mapping_mode {
    shared,
    private_copy
};

inline std::expected<window_plan, std::error_code> plan_readonly_window(
    std::uint64_t, std::uint64_t, std::size_t, std::size_t) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}

inline std::expected<c07::readonly_mapping, std::error_code> map_window(
    const std::filesystem::path&, std::uint64_t, std::size_t) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}

inline std::expected<void, std::error_code> write_first_byte(
    const std::filesystem::path&, write_mapping_mode, char) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}

} // namespace c07_l04

#endif
