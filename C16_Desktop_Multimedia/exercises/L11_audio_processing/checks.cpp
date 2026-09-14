#include <solution.hpp>
#include <c16/check.hpp>

#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

bool close(float a, float b, float eps = 0.0005f) { return std::fabs(a - b) <= eps; }

void check_gain_mix_metrics()
{
    using namespace c16_l11;
    const auto gained = apply_gain_clamped({0.25f, -0.75f, 0.8f}, 2.0f);
    c16::require(gained == std::vector<float>({0.5f, -1.0f, 1.0f}), "gain clamps to [-1,1]");

    const auto mixed = mix_clamped({0.4f, -0.7f}, {0.7f, -0.6f});
    c16::require(mixed == std::vector<float>({1.0f, -1.0f}), "mix clamps summed samples");

    const Metrics m = measure({-0.5f, 0.25f, 1.0f, -1.0f});
    c16::require(close(m.rms, std::sqrt((0.25f + 0.0625f + 1.0f + 1.0f) / 4.0f)), "RMS uses mean square");
    c16::require(close(m.peak, 1.0f), "peak is max absolute sample");
}

void check_waveform_and_resample()
{
    using namespace c16_l11;
    const auto wave = reduce_minmax({0.0f, 0.5f, -0.25f, 0.75f, -1.0f}, 2);
    c16::require(wave.size() == 3, "tail waveform bucket kept");
    c16::require(close(wave[0].min, 0.0f) && close(wave[0].max, 0.5f), "first minmax bucket");
    c16::require(close(wave[2].min, -1.0f) && close(wave[2].max, -1.0f), "tail minmax bucket");

    const auto resampled = resample_linear({0.0f, 10.0f, 20.0f}, 5);
    c16::require(resampled.size() == 5, "resampler returns requested length");
    c16::require(close(resampled[0], 0.0f) && close(resampled[2], 10.0f) && close(resampled[4], 20.0f),
        "linear resampling preserves endpoints and middle");

    const auto constant = resample_linear({0.25f}, 4);
    c16::require(constant == std::vector<float>({0.25f, 0.25f, 0.25f, 0.25f}),
        "single input sample repeats to requested output length");
}

void check_invalid_numbers()
{
    using namespace c16_l11;
    bool rejected_nan = false;
    try {
        (void)measure({0.0f, std::numeric_limits<float>::quiet_NaN()});
    } catch (const std::invalid_argument&) {
        rejected_nan = true;
    }
    c16::require(rejected_nan, "NaN input is rejected");

    bool rejected_inf_gain = false;
    try {
        (void)apply_gain_clamped({0.25f}, std::numeric_limits<float>::infinity());
    } catch (const std::invalid_argument&) {
        rejected_inf_gain = true;
    }
    c16::require(rejected_inf_gain, "infinite gain is rejected");
}

} // namespace

int main()
{
    return c16::run([] {
        check_gain_mix_metrics();
        check_waveform_and_resample();
        check_invalid_numbers();
    });
}
