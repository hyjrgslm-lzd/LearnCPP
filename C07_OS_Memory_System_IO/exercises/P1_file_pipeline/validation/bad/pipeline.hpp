#pragma once
#include <c07/file_pipeline_types.hpp>
#include <cstddef>
#include <expected>
#include <span>
#include <system_error>
#include <vector>

namespace c07_p1 {
inline std::expected<std::vector<std::byte>, std::error_code> assemble(
    std::span<const c07::completed_chunk> chunks, std::size_t total_size) {
    if (total_size > c07::max_pipeline_bytes || chunks.size() > c07::max_pipeline_chunks) {
        return std::unexpected(std::make_error_code(std::errc::value_too_large));
    }
    std::vector<std::byte> result;
    result.reserve(total_size);
    for (const auto& chunk : chunks) {
        result.insert(result.end(), chunk.bytes.begin(), chunk.bytes.end());
    }
    if (result.size() != total_size) return std::unexpected(std::make_error_code(std::errc::protocol_error));
    return result;
}
} // namespace c07_p1
