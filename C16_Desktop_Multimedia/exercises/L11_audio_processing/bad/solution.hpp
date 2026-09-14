#pragma once

#include <cmath>
#include <vector>

namespace c16_l11 {

struct Metrics { float rms = 0.0f; float peak = 0.0f; };
struct MinMax { float min = 0.0f; float max = 0.0f; };

inline std::vector<float> apply_gain_clamped(const std::vector<float>& input, float gain)
{
    std::vector<float> out;
    for (float sample : input) out.push_back(sample * gain);
    return out;
}
inline std::vector<float> mix_clamped(const std::vector<float>& a, const std::vector<float>& b)
{
    std::vector<float> out;
    for (std::size_t i = 0; i < a.size() && i < b.size(); ++i) out.push_back(a[i] + b[i]);
    return out;
}
inline Metrics measure(const std::vector<float>& input)
{
    float sum = 0.0f;
    for (float sample : input) sum += std::fabs(sample);
    return {input.empty() ? 0.0f : sum / input.size(), sum};
}
inline std::vector<MinMax> reduce_minmax(const std::vector<float>& input, int)
{
    return input.empty() ? std::vector<MinMax>{} : std::vector<MinMax>{{input.front(), input.back()}};
}
inline std::vector<float> resample_linear(const std::vector<float>& input, int output_count)
{
    return std::vector<float>(static_cast<std::size_t>(output_count), input.empty() ? 0.0f : input.front());
}

} // namespace c16_l11
