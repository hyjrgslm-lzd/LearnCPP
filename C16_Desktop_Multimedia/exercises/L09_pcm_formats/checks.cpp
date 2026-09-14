#include <solution.hpp>
#include <c16/check.hpp>

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

void check_pcm_math()
{
    using namespace c16_l09;
    const PcmFormat stereo{48000, 2, 2};
    c16::require(bytes_per_frame(stereo) == 4, "stereo s16 frame is 4 bytes");
    c16::require(frames_for_bytes(stereo, 16) == 4, "bytes to frames uses channels");
    c16::require(bytes_for_frames(stereo, 4) == 16, "frames to bytes uses channels");
    c16::require(duration_us_for_frames(stereo, 48000) == 1000000, "one second duration");
    c16::require(frame_floor_for_time_us(stereo, 250000) == 12000, "time to frame floors safely");
}

void check_tail_and_channels()
{
    using namespace c16_l09;
    const PcmFormat stereo{48000, 2, 2};
    std::vector<std::int16_t> interleaved{100, -100, 200, -200, 300, -300};
    auto split = split_stereo(interleaved);
    c16::require(split.left == std::vector<std::int16_t>({100, 200, 300}), "left channel extracted");
    c16::require(split.right == std::vector<std::int16_t>({-100, -200, -300}), "right channel extracted");

    bool rejected_tail = false;
    try {
        (void)frames_for_bytes(stereo, 18);
    } catch (const std::invalid_argument&) {
        rejected_tail = true;
    }
    c16::require(rejected_tail, "partial frame tail is rejected");
}

void check_overflow()
{
    using namespace c16_l09;
    const PcmFormat huge{192000, 8, 8};
    bool overflow = false;
    try {
        (void)bytes_for_frames(huge, std::numeric_limits<std::uint64_t>::max());
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    c16::require(overflow, "byte count overflow is reported");
    c16::require(duration_us_for_frames(PcmFormat{44100, 1, 2}, 441) == 10000,
        "duration conversion keeps integer precision before division");
}

} // namespace

int main()
{
    return c16::run([] {
        check_pcm_math();
        check_tail_and_channels();
        check_overflow();
    });
}
