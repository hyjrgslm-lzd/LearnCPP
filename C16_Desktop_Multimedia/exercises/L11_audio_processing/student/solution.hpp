#pragma once

#include <vector>

namespace c16_l11 {

struct Metrics { float rms = 0.0f; float peak = 0.0f; };
struct MinMax { float min = 0.0f; float max = 0.0f; };

inline int todo_value() { static volatile int value = 0; return value; }
inline std::vector<float> todo_samples()
{
    std::vector<float> out;
    if (todo_value() != 0) out.push_back(1.0f);
    return out;
}
inline std::vector<float> apply_gain_clamped(const std::vector<float>&, float) { return todo_samples(); }
inline std::vector<float> mix_clamped(const std::vector<float>&, const std::vector<float>&) { return todo_samples(); }
inline Metrics measure(const std::vector<float>&) { return {static_cast<float>(todo_value()), 0.0f}; }
inline std::vector<MinMax> reduce_minmax(const std::vector<float>&, int)
{
    std::vector<MinMax> out;
    if (todo_value() != 0) out.push_back({});
    return out;
}
inline std::vector<float> resample_linear(const std::vector<float>&, int) { return todo_samples(); }

} // namespace c16_l11
