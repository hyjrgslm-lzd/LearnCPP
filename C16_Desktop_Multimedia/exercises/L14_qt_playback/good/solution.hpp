#pragma once

#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioFormat>
#include <QColor>
#include <QEventLoop>
#include <QImage>
#include <QMediaPlayer>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>

#include <algorithm>
#include <cmath>

namespace c16_l14 {

struct DecodeProbe {
    bool saw_audio = false;
    bool saw_video = false;
    int audio_buffers = 0;
    qint64 audio_frames = 0;
    int audio_sample_rate = 0;
    int audio_channels = 0;
    double max_abs_sample = 0.0;
    double pulse1_peak = 0.0;
    double pulse2_peak = 0.0;
    double pulse3_peak = 0.0;
    double quiet_peak = 0.0;
    qint64 first_audio_start = -1;
    int video_frames = 0;
    QSize first_video_size;
    qint64 first_video_start = -1;
    int video_byte_variation = 0;
    int video_pattern_matches = 0;
    bool eof = false;
    bool error = false;
    bool timed_out = false;
    QString error_string;
};

inline DecodeProbe probe_media(const QString& path, int timeout_ms)
{
    DecodeProbe out;
    QMediaPlayer player;
    QAudioBufferOutput audio;
    QVideoSink sink;
    QEventLoop wait;
    bool finished = false;
    player.setAudioBufferOutput(&audio);
    player.setVideoSink(&sink);
    qint64 sample_index = 0;
    auto add_sample = [&](double v) {
        const double a = std::abs(v);
        out.max_abs_sample = std::max(out.max_abs_sample, a);
        auto near = [&](qint64 c) { return sample_index >= c - 120 && sample_index < c + 120; };
        if (near(4800)) out.pulse1_peak = std::max(out.pulse1_peak, a);
        if (near(24000)) out.pulse2_peak = std::max(out.pulse2_peak, a);
        if (near(43200)) out.pulse3_peak = std::max(out.pulse3_peak, a);
        if (sample_index >= 1000 && sample_index < 1200) out.quiet_peak = std::max(out.quiet_peak, a);
        ++sample_index;
    };
    auto read_audio = [&](const QAudioBuffer& b) {
        if (b.format().sampleFormat() == QAudioFormat::Int16) {
            const auto* data = b.constData<qint16>();
            for (qsizetype i = 0; i < b.sampleCount(); ++i) add_sample(data[i] / 32768.0);
        } else if (b.format().sampleFormat() == QAudioFormat::Float) {
            const auto* data = b.constData<float>();
            for (qsizetype i = 0; i < b.sampleCount(); ++i) add_sample(static_cast<double>(data[i]));
        } else if (b.format().sampleFormat() == QAudioFormat::UInt8) {
            const auto* data = b.constData<quint8>();
            for (qsizetype i = 0; i < b.sampleCount(); ++i) add_sample((static_cast<int>(data[i]) - 128) / 128.0);
        } else if (b.format().sampleFormat() == QAudioFormat::Int32) {
            const auto* data = b.constData<qint32>();
            for (qsizetype i = 0; i < b.sampleCount(); ++i) add_sample(data[i] / 2147483648.0);
        }
    };
    auto rgb_match = [](const QColor& c, int r, int g, int b) {
        return std::abs(c.red() - r) <= 10 && std::abs(c.green() - g) <= 10 && std::abs(c.blue() - b) <= 10;
    };

    QObject::connect(&audio, &QAudioBufferOutput::audioBufferReceived, &wait, [&](const QAudioBuffer& b) {
        if (b.frameCount() < 1) return;
        out.saw_audio = true;
        ++out.audio_buffers;
        out.audio_frames += b.frameCount();
        out.audio_sample_rate = b.format().sampleRate();
        out.audio_channels = b.format().channelCount();
        read_audio(b);
        if (out.first_audio_start < 0) out.first_audio_start = b.startTime();
    });
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &wait, [&](const QVideoFrame& f) {
        if (!f.isValid()) return;
        out.saw_video = true;
        ++out.video_frames;
        if (out.video_frames == 1) {
            out.first_video_size = f.size();
            out.first_video_start = f.startTime();
            QVideoFrame copy(f);
            if (copy.map(QVideoFrame::ReadOnly) && copy.planeCount()) {
                const auto* data = copy.bits(0);
                const int n = std::min(copy.mappedBytes(0), 4096);
                int lo = 255;
                int hi = 0;
                for (int i = 0; i < n; ++i) {
                    lo = std::min(lo, static_cast<int>(data[i]));
                    hi = std::max(hi, static_cast<int>(data[i]));
                }
                out.video_byte_variation = hi - lo;
                copy.unmap();
            }
            const QImage image = f.toImage();
            if (!image.isNull() && image.width() >= 21 && image.height() >= 21) {
                out.video_pattern_matches += rgb_match(image.pixelColor(0, 0), 0, 0, 255) ? 1 : 0;
                out.video_pattern_matches += rgb_match(image.pixelColor(20, 0), 60, 0, 32) ? 1 : 0;
                out.video_pattern_matches += rgb_match(image.pixelColor(0, 20), 0, 100, 32) ? 1 : 0;
                out.video_pattern_matches += rgb_match(image.pixelColor(20, 20), 60, 100, 255) ? 1 : 0;
            }
        }
    });
    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &wait, [&](QMediaPlayer::MediaStatus s) {
        if (s == QMediaPlayer::EndOfMedia) {
            out.eof = true;
            finished = true;
            wait.quit();
        }
        if (s == QMediaPlayer::InvalidMedia) {
            out.error = true;
            out.error_string = player.errorString();
            finished = true;
            wait.quit();
        }
    });
    QObject::connect(&player, &QMediaPlayer::errorChanged, &wait, [&] {
        if (player.error() != QMediaPlayer::NoError) {
            out.error = true;
            out.error_string = player.errorString();
            finished = true;
            wait.quit();
        }
    });
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &wait, [&] {
        if (!finished) {
            out.error = true;
            out.timed_out = true;
            out.error_string = "decode timeout";
            finished = true;
            wait.quit();
        }
    });
    player.setSource(QUrl::fromLocalFile(path));
    player.play();
    timeout.start(timeout_ms);
    if (!finished) {
        wait.exec();
    }
    player.stop();
    return out;
}

} // namespace c16_l14
