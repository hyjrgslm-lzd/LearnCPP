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

inline std::uint64_t bytes_per_frame(PcmFormat format)
{
    if (format.sample_rate == 0 || format.channels == 0 || format.bytes_per_sample == 0) {
        throw std::invalid_argument("invalid PCM format");
    }
    const auto channels = static_cast<std::uint64_t>(format.channels);
    const auto bytes = static_cast<std::uint64_t>(format.bytes_per_sample);
    if (channels > std::numeric_limits<std::uint64_t>::max() / bytes) {
        throw std::overflow_error("PCM frame size overflow");
    }
    return channels * bytes;
}

inline std::uint64_t frames_for_bytes(PcmFormat format, std::uint64_t bytes)
{
    const auto frame = bytes_per_frame(format);
    if (bytes % frame != 0) {
        throw std::invalid_argument("buffer ends in a partial PCM frame");
    }
    return bytes / frame;
}

inline std::uint64_t bytes_for_frames(PcmFormat format, std::uint64_t frames)
{
    const auto frame = bytes_per_frame(format);
    if (frames > std::numeric_limits<std::uint64_t>::max() / frame) {
        throw std::overflow_error("PCM byte count overflow");
    }
    return frames * frame;
}

inline std::uint64_t duration_us_for_frames(PcmFormat format, std::uint64_t frames)
{
    if (format.sample_rate == 0 || frames > std::numeric_limits<std::uint64_t>::max() / 1000000ULL) {
        throw std::overflow_error("PCM duration overflow");
    }
    return (frames * 1000000ULL) / format.sample_rate;
}

inline std::uint64_t frame_floor_for_time_us(PcmFormat format, std::uint64_t microseconds)
{
    if (format.sample_rate == 0 || microseconds > std::numeric_limits<std::uint64_t>::max() / format.sample_rate) {
        throw std::overflow_error("PCM time conversion overflow");
    }
    return (microseconds * format.sample_rate) / 1000000ULL;
}

inline StereoSplit split_stereo(const std::vector<std::int16_t>& interleaved)
{
    if (interleaved.size() % 2 != 0) {
        throw std::invalid_argument("stereo data must contain complete frames");
    }
    StereoSplit out;
    out.left.reserve(interleaved.size() / 2);
    out.right.reserve(interleaved.size() / 2);
    for (std::size_t i = 0; i < interleaved.size(); i += 2) {
        out.left.push_back(interleaved[i]);
        out.right.push_back(interleaved[i + 1]);
    }
    return out;
}

} // namespace c16_l09
