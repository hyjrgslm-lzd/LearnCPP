#ifndef C07_L03_VIRTUAL_MEMORY_LAB_HPP
#define C07_L03_VIRTUAL_MEMORY_LAB_HPP

#include <c07/memory.hpp>

#include <cstddef>
#include <expected>
#include <system_error>

namespace c07_l03 {

inline std::expected<c07::virtual_region, std::error_code> make_region(std::size_t, std::byte) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}

} // namespace c07_l03

#endif
