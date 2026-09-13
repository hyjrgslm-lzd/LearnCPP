#pragma once
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace c11 {
inline constexpr std::size_t max_frame = 4096;
enum class frame_error { bad_length, too_large, truncated, unfinished };
using frame_step = std::expected<std::optional<std::string>, frame_error>;

inline std::expected<std::string, frame_error> encode_frame(std::string_view body) {
    if (body.size() > max_frame) return std::unexpected(frame_error::too_large);
    std::string header(8, '0');
    auto length = body.size();
    for (int i = 7; i >= 0; --i) {
        header[static_cast<std::size_t>(i)] += static_cast<char>(length % 10);
        length /= 10;
    }
    header.append(body);
    return header;
}
} // namespace c11
