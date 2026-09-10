#ifndef C07_L03_VIRTUAL_MEMORY_LAB_HPP
#define C07_L03_VIRTUAL_MEMORY_LAB_HPP

#include <c07/memory.hpp>

#include <cstddef>

namespace c07_l03 {

inline std::expected<c07::virtual_region, std::error_code> make_region(std::size_t pages, std::byte fill) {
    const auto p = c07::query_page_info();
    if (p.page_size == 0 || pages == 0) return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    auto region = c07::virtual_region::reserve(p.page_size * pages);
    if (!region) return std::unexpected(region.error());
    auto committed = region->commit(0, p.page_size, c07::page_access::read_write);
    if (!committed) return std::unexpected(committed.error());
    auto bytes = region->bytes(0, p.page_size);
    bytes.front() = fill;
    bytes.back() = fill;
    return std::move(*region);
}

} // namespace c07_l03

#endif
