#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace c16_l11 {

struct Metrics { float rms = 0.0f; float peak = 0.0f; };
struct MinMax { float min = 0.0f; float max = 0.0f; };

inline void check_finite(float value)
{
    if (!std::isfinite(value)) {
        throw std::invalid_argument("non-finite audio value");
    }
}

inline void check_all_finite(const std::vector<float>& samples)
{
    for (float sample : samples) {
        check_finite(sample);
    }
}

inline std::vector<float> apply_gain_clamped(const std::vector<float>& samples, float gain)
{
    check_finite(gain);
    check_all_finite(samples);
    std::vector<float> out(samples.size());
    std::transform(samples.begin(), samples.end(), out.begin(),
        [gain](float sample) { return std::clamp(sample * gain, -1.0f, 1.0f); });
    return out;
}

inline std::vector<float> mix_clamped(const std::vector<float>& left, const std::vector<float>& right)
{
    if (left.size() != right.size()) {
        throw std::invalid_argument("different stream lengths");
    }
    check_all_finite(left);
    check_all_finite(right);
    std::vector<float> out(left.size());
    for (std::size_t i = 0; i < left.size(); ++i) {
        out[i] = std::clamp(left[i] + right[i], -1.0f, 1.0f);
    }
    return out;
}

inline Metrics measure(const std::vector<float>& samples)
{
    double energy = 0.0;
    check_all_finite(samples);
    float peak = 0.0f;
    for (float sample : samples) {
        energy += sample * sample;
        peak = std::max(peak, std::abs(sample));
    }
    return samples.empty() ? Metrics{} : Metrics{static_cast<float>(std::sqrt(energy / samples.size())), peak};
}

inline std::vector<MinMax> reduce_minmax(const std::vector<float>& samples, int bucket)
{
    if (bucket < 1) {
        throw std::invalid_argument("bucket");
    }
    check_all_finite(samples);
    std::vector<MinMax> out;
    for (std::size_t i = 0; i < samples.size(); i += static_cast<std::size_t>(bucket)) {
        float lo = samples[i];
        float hi = samples[i];
        for (std::size_t j = i; j < samples.size() && j < i + static_cast<std::size_t>(bucket); ++j) {
            lo = std::min(lo, samples[j]);
            hi = std::max(hi, samples[j]);
        }
        out.push_back({lo, hi});
    }
    return out;
}

inline std::vector<float> resample_linear(const std::vector<float>& samples, int count)
{
    if (count < 0) {
        throw std::invalid_argument("count");
    }
    if (samples.empty() || count == 0) {
        return {};
    }
    check_all_finite(samples);
    if (count == 1) {
        return {samples.front()};
    }
    if (samples.size() == 1) {
        return std::vector<float>(static_cast<std::size_t>(count), samples.front());
    }
    std::vector<float> out;
    out.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const double x = static_cast<double>(i) * (samples.size() - 1) / (count - 1);
        const auto lo = static_cast<std::size_t>(x);
        const auto hi = std::min(lo + 1, samples.size() - 1);
        const auto frac = static_cast<float>(x - lo);
        out.push_back(samples[lo] + (samples[hi] - samples[lo]) * frac);
    }
    return out;
}

} // namespace c16_l11
