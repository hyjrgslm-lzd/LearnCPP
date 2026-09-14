#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace c16_l11 {

struct Metrics { float rms = 0.0f; float peak = 0.0f; };
struct MinMax { float min = 0.0f; float max = 0.0f; };

inline void require_finite(float value)
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument("audio sample must be finite");
    }
}

inline void require_finite_samples(const std::vector<float>& input)
{
    for (float sample : input) {
        require_finite(sample);
    }
}

inline float clamp_sample(float value) { return std::clamp(value, -1.0f, 1.0f); }

inline std::vector<float> apply_gain_clamped(const std::vector<float>& input, float gain)
{
    require_finite(gain);
    require_finite_samples(input);
    std::vector<float> out;
    out.reserve(input.size());
    for (float sample : input) {
        out.push_back(clamp_sample(sample * gain));
    }
    return out;
}

inline std::vector<float> mix_clamped(const std::vector<float>& a, const std::vector<float>& b)
{
    if (a.size() != b.size()) {
        throw std::invalid_argument("mix inputs must have same length");
    }
    require_finite_samples(a);
    require_finite_samples(b);
    std::vector<float> out;
    out.reserve(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        out.push_back(clamp_sample(a[i] + b[i]));
    }
    return out;
}

inline Metrics measure(const std::vector<float>& input)
{
    if (input.empty()) {
        return {};
    }
    require_finite_samples(input);
    double squares = 0.0;
    float peak = 0.0f;
    for (float sample : input) {
        squares += static_cast<double>(sample) * sample;
        peak = std::max(peak, std::fabs(sample));
    }
    return {static_cast<float>(std::sqrt(squares / input.size())), peak};
}

inline std::vector<MinMax> reduce_minmax(const std::vector<float>& input, int bucket_size)
{
    if (bucket_size <= 0) {
        throw std::invalid_argument("bucket size must be positive");
    }
    require_finite_samples(input);
    std::vector<MinMax> out;
    for (std::size_t first = 0; first < input.size(); first += static_cast<std::size_t>(bucket_size)) {
        const auto last = std::min(input.size(), first + static_cast<std::size_t>(bucket_size));
        auto [min_it, max_it] = std::minmax_element(input.begin() + static_cast<std::ptrdiff_t>(first),
            input.begin() + static_cast<std::ptrdiff_t>(last));
        out.push_back({*min_it, *max_it});
    }
    return out;
}

inline std::vector<float> resample_linear(const std::vector<float>& input, int output_count)
{
    if (output_count < 0) {
        throw std::invalid_argument("negative output count");
    }
    if (input.empty() || output_count == 0) {
        return {};
    }
    require_finite_samples(input);
    if (output_count == 1) {
        return {input.front()};
    }
    if (input.size() == 1) {
        return std::vector<float>(static_cast<std::size_t>(output_count), input.front());
    }
    std::vector<float> out(static_cast<std::size_t>(output_count));
    const double scale = static_cast<double>(input.size() - 1) / static_cast<double>(output_count - 1);
    for (int i = 0; i < output_count; ++i) {
        const double source = i * scale;
        const auto left = static_cast<std::size_t>(std::floor(source));
        const auto right = std::min(left + 1, input.size() - 1);
        const float t = static_cast<float>(source - left);
        out[static_cast<std::size_t>(i)] = input[left] * (1.0f - t) + input[right] * t;
    }
    return out;
}

} // namespace c16_l11
