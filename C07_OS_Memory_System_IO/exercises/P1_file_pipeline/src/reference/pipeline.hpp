#pragma once
#include <c07/file_pipeline_types.hpp>
#include <algorithm>
#include <cstddef>
#include <expected>
#include <limits>
#include <span>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace c07_p1 {
namespace detail {
inline std::unexpected<std::error_code> protocol_error() {
    return std::unexpected(std::make_error_code(std::errc::protocol_error));
}
} // namespace detail

inline std::expected<std::vector<std::byte>, std::error_code> assemble(
    std::span<const c07::completed_chunk> chunks, std::size_t total_size) {
    if (total_size > c07::max_pipeline_bytes || chunks.size() > c07::max_pipeline_chunks) {
        return std::unexpected(std::make_error_code(std::errc::value_too_large));
    }
    if (total_size == 0) {
        return chunks.empty() ? std::expected<std::vector<std::byte>, std::error_code>{std::vector<std::byte>{}}
                              : detail::protocol_error();
    }

    std::unordered_set<std::uint64_t> ids;
    ids.reserve(chunks.size());
    std::vector<std::size_t> order(chunks.size());
    for (std::size_t i = 0; i != chunks.size(); ++i) {
        const auto& chunk = chunks[i];
        if (chunk.request_id == 0 || chunk.bytes.empty() || !ids.insert(chunk.request_id).second) {
            return detail::protocol_error();
        }
        if (chunk.offset > std::numeric_limits<std::size_t>::max() - chunk.bytes.size()) {
            return detail::protocol_error();
        }
        if (chunk.offset + chunk.bytes.size() > total_size) return detail::protocol_error();
        order[i] = i;
    }
    std::ranges::sort(order, [&](std::size_t lhs, std::size_t rhs) {
        return chunks[lhs].offset < chunks[rhs].offset;
    });

    std::size_t expected_offset = 0;
    for (const std::size_t index : order) {
        const auto& chunk = chunks[index];
        if (chunk.offset != expected_offset) return detail::protocol_error();
        expected_offset += chunk.bytes.size();
    }
    if (expected_offset != total_size) return detail::protocol_error();

    std::vector<std::byte> result(total_size);
    for (const std::size_t index : order) {
        const auto& chunk = chunks[index];
        std::ranges::copy(chunk.bytes, result.begin() + static_cast<std::ptrdiff_t>(chunk.offset));
    }
    return result;
}
} // namespace c07_p1
