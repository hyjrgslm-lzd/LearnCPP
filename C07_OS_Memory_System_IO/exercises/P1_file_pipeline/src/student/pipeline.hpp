#pragma once
#include <c07/file_pipeline_types.hpp>
#include <expected>
#include <system_error>
#include <vector>

namespace c07_p1 {
inline std::expected<std::vector<std::byte>, std::error_code> assemble(
    std::span<const c07::completed_chunk>, std::size_t) {
    return std::unexpected(std::make_error_code(std::errc::function_not_supported));
}
} // namespace c07_p1
