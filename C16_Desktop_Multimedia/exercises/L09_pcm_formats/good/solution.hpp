#pragma once

#include <cstdint>
#include <limits>
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

inline std::uint64_t checked_mul(std::uint64_t a, std::uint64_t b, const char* what)
{
    if (b != 0 && a > std::numeric_limits<std::uint64_t>::max() / b) {
        throw std::overflow_error(what);
    }
    return a * b;
}

inline std::uint64_t bytes_per_frame(PcmFormat f)
{
    if (!f.sample_rate || !f.channels || !f.bytes_per_sample) {
        throw std::invalid_argument("bad pcm format");
    }
    return checked_mul(f.channels, f.bytes_per_sample, "frame bytes overflow");
}

inline std::uint64_t frames_for_bytes(PcmFormat f, std::uint64_t bytes)
{
    const auto stride = bytes_per_frame(f);
    if (bytes % stride) {
        throw std::invalid_argument("incomplete frame");
    }
    return bytes / stride;
}

inline std::uint64_t bytes_for_frames(PcmFormat f, std::uint64_t frames)
{
    return checked_mul(frames, bytes_per_frame(f), "buffer bytes overflow");
}

inline std::uint64_t duration_us_for_frames(PcmFormat f, std::uint64_t frames)
{
    if (!f.sample_rate) {
        throw std::invalid_argument("sample rate required");
    }
    return checked_mul(frames, 1000000ULL, "duration overflow") / f.sample_rate;
}

inline std::uint64_t frame_floor_for_time_us(PcmFormat f, std::uint64_t us)
{
    if (!f.sample_rate) {
        throw std::invalid_argument("sample rate required");
    }
    return checked_mul(us, f.sample_rate, "time overflow") / 1000000ULL;
}

inline StereoSplit split_stereo(const std::vector<std::int16_t>& data)
{
    if (data.size() % 2) {
        throw std::invalid_argument("odd sample count");
    }
    StereoSplit split;
    for (std::size_t frame = 0; frame < data.size() / 2; ++frame) {
        split.left.push_back(data[frame * 2]);
        split.right.push_back(data[frame * 2 + 1]);
    }
    return split;
}

} // namespace c16_l09
