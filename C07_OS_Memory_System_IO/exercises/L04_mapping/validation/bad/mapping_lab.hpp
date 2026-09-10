#ifndef C07_L04_MAPPING_LAB_HPP
#define C07_L04_MAPPING_LAB_HPP

#include <c07/memory.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <system_error>

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
    std::uint64_t, std::uint64_t offset, std::size_t length, std::size_t) {
    return window_plan{offset, 0, length, length};
}

inline std::expected<c07::readonly_mapping, std::error_code> map_window(
    const std::filesystem::path& path, std::uint64_t offset, std::size_t length) {
    return c07::map_readonly(path, offset, length);
}

inline std::expected<void, std::error_code> write_first_byte(
    const std::filesystem::path&, write_mapping_mode, char) {
    return {};
}

} // namespace c07_l04

#endif
