#pragma once

#include <cstdint>
#include <stdexcept>
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

inline std::uint64_t bytes_per_frame(PcmFormat format) { return format.bytes_per_sample; }
inline std::uint64_t frames_for_bytes(PcmFormat format, std::uint64_t bytes) { return bytes / bytes_per_frame(format); }
inline std::uint64_t bytes_for_frames(PcmFormat format, std::uint64_t frames) { return frames * bytes_per_frame(format); }
inline std::uint64_t duration_us_for_frames(PcmFormat format, std::uint64_t frames)
{
    return (frames / format.sample_rate) * 1000000ULL;
}
inline std::uint64_t frame_floor_for_time_us(PcmFormat format, std::uint64_t us) { return us * format.sample_rate / 1000000ULL; }
inline StereoSplit split_stereo(const std::vector<std::int16_t>& interleaved)
{
    StereoSplit out;
    out.left = interleaved;
    return out;
}

} // namespace c16_l09
