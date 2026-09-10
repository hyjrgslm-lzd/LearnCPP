#pragma once
#include <c07/file_pipeline_types.hpp>
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
    std::vector<bool> covered(total_size, false);
    std::size_t covered_count = 0;
    for (const auto& chunk : chunks) {
        if (chunk.request_id == 0 || chunk.bytes.empty() || !ids.insert(chunk.request_id).second) {
            return detail::protocol_error();
        }
        if (chunk.offset > std::numeric_limits<std::size_t>::max() - chunk.bytes.size()) {
            return detail::protocol_error();
        }
        const std::size_t end = chunk.offset + chunk.bytes.size();
        if (end > total_size) return detail::protocol_error();
        for (std::size_t pos = chunk.offset; pos != end; ++pos) {
            if (covered[pos]) return detail::protocol_error();
            covered[pos] = true;
            ++covered_count;
        }
    }
    if (covered_count != total_size) return detail::protocol_error();

    std::vector<std::byte> result(total_size);
    for (const auto& chunk : chunks) {
        for (std::size_t i = 0; i != chunk.bytes.size(); ++i) result[chunk.offset + i] = chunk.bytes[i];
    }
    return result;
}
} // namespace c07_p1
