#pragma once

#include <cstdint>
#include <limits>
#include <vector>

namespace c16_l09 {

struct PcmFormat {
    std::uint32_t sample_rate = 0;
    std::uint16_t channels = 0;
    std::uint16_t bytes_per_sample = 0;
};

struct StereoSplit {
    std::vector<std::int16_t> left;
    std::vector<std::int16_t> right;
};

inline int todo_value() { static volatile int value = 0; return value; }
inline std::uint64_t bytes_per_frame(PcmFormat) { return static_cast<std::uint64_t>(todo_value()); }
inline std::uint64_t frames_for_bytes(PcmFormat, std::uint64_t) { return static_cast<std::uint64_t>(todo_value()); }
inline std::uint64_t bytes_for_frames(PcmFormat, std::uint64_t) { return static_cast<std::uint64_t>(todo_value()); }
inline std::uint64_t duration_us_for_frames(PcmFormat, std::uint64_t) { return static_cast<std::uint64_t>(todo_value()); }
inline std::uint64_t frame_floor_for_time_us(PcmFormat, std::uint64_t) { return static_cast<std::uint64_t>(todo_value()); }
inline StereoSplit split_stereo(const std::vector<std::int16_t>&)
{
    StereoSplit out;
    if (todo_value() != 0) out.left.push_back(1);
    return out;
}

} // namespace c16_l09
