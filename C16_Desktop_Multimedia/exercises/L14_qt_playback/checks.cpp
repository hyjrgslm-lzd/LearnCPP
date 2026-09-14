#include <solution.hpp>
#include <c16/check.hpp>

#include <QCoreApplication>
#include <QFileInfo>

#include <filesystem>
#include <cmath>

namespace {

QString fixture_path(const char* name)
{
    const auto source = std::filesystem::path(__FILE__).parent_path();
    const auto course = source.parent_path().parent_path();
    return QString::fromStdWString((course / "build" / "fixtures" / name).wstring());
}

void check_audio_oracle(const c16_l14::DecodeProbe& probe)
{
    c16::require(probe.audio_sample_rate == 48000, "fixture decodes as 48 kHz audio");
    c16::require(probe.audio_channels == 1, "fixture decodes as mono audio");
    c16::require(probe.audio_frames == 48000, "fixture decodes exactly one second of PCM frames");
    c16::require(probe.first_audio_start >= -1 && probe.first_audio_start <= 50000,
        "first audio timestamp starts near stream origin");
    c16::require(probe.pulse1_peak > 0.55 && probe.pulse2_peak > 0.55 && probe.pulse3_peak > 0.55,
        "decoded PCM contains all three generated pulse windows");
    c16::require(probe.quiet_peak > 0.04 && probe.quiet_peak < 0.08,
        "decoded PCM quiet window matches generated 440 Hz tone scale");
}

void check_wav_audio_decode()
{
    const auto path = fixture_path("c16_pulse_mono_48k_s16.wav");
    c16::require(QFileInfo::exists(path), "wav fixture exists");
    const auto probe = c16_l14::probe_media(path, 5000);
    c16::require(!probe.timed_out, "wav decode must not time out");
    c16::require(!probe.error, "wav decode has no media error: " + probe.error_string.toStdString());
    c16::require(probe.eof, "wav reaches EOF");
    c16::require(probe.saw_audio && probe.audio_buffers > 0, "wav delivers decoded audio buffers");
    check_audio_oracle(probe);
}

void check_avi_audio_video_decode()
{
    const auto path = fixture_path("c16_rgb24_pcm_1s.avi");
    c16::require(QFileInfo::exists(path), "avi fixture exists");
    const auto probe = c16_l14::probe_media(path, 7000);
    c16::require(!probe.timed_out, "avi decode must not time out");
    c16::require(!probe.error, "avi decode has no media error: " + probe.error_string.toStdString());
    c16::require(probe.eof, "avi reaches EOF");
    c16::require(probe.saw_audio && probe.audio_frames > 0, "avi delivers decoded audio");
    c16::require(probe.saw_video && probe.video_frames > 0, "avi delivers decoded video frames");
    check_audio_oracle(probe);
    c16::require(probe.first_video_size.width() == 64 && probe.first_video_size.height() == 48,
        "avi decoded frame keeps fixture size");
    c16::require(probe.first_video_start >= -1 && probe.first_video_start <= 50000,
        "first video timestamp starts near stream origin");
    c16::require(probe.video_pattern_matches >= 3, "avi decoded first frame matches generated color-block oracle");
}

void check_bad_inputs()
{
    const auto empty = c16_l14::probe_media(fixture_path("c16_empty.media"), 3000);
    c16::require(!empty.timed_out, "empty media rejection must come from media error, not timeout");
    c16::require(empty.error, "empty media reports decode error");
    const auto truncated = c16_l14::probe_media(fixture_path("c16_truncated.avi"), 3000);
    c16::require(!truncated.timed_out, "truncated avi rejection must come from media error, not timeout");
    c16::require(truncated.error, "truncated avi reports decode error");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        check_wav_audio_decode();
        check_avi_audio_video_decode();
        check_bad_inputs();
    });
}
