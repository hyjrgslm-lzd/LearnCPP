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
    DecodeProbe result;
    QMediaPlayer player;
    QAudioBufferOutput audio_tap;
    QVideoSink video_sink;
    QEventLoop loop;
    bool finished = false;

    player.setAudioBufferOutput(&audio_tap);
    player.setVideoSink(&video_sink);

    qint64 sample_index = 0;
    auto record_sample = [&](double value) {
        const double absolute = std::abs(value);
        result.max_abs_sample = std::max(result.max_abs_sample, absolute);
        auto in_window = [&](qint64 center) { return sample_index >= center - 120 && sample_index < center + 120; };
        if (in_window(4800)) result.pulse1_peak = std::max(result.pulse1_peak, absolute);
        if (in_window(24000)) result.pulse2_peak = std::max(result.pulse2_peak, absolute);
        if (in_window(43200)) result.pulse3_peak = std::max(result.pulse3_peak, absolute);
        if (sample_index >= 1000 && sample_index < 1200) result.quiet_peak = std::max(result.quiet_peak, absolute);
        ++sample_index;
    };
    auto consume_samples = [&](const QAudioBuffer& buffer) {
        switch (buffer.format().sampleFormat()) {
        case QAudioFormat::UInt8: {
            const auto* data = buffer.constData<quint8>();
            for (qsizetype i = 0; i < buffer.sampleCount(); ++i) {
                record_sample((static_cast<int>(data[i]) - 128) / 128.0);
            }
            break;
        }
        case QAudioFormat::Int16: {
            const auto* data = buffer.constData<qint16>();
            for (qsizetype i = 0; i < buffer.sampleCount(); ++i) {
                record_sample(data[i] / 32768.0);
            }
            break;
        }
        case QAudioFormat::Int32: {
            const auto* data = buffer.constData<qint32>();
            for (qsizetype i = 0; i < buffer.sampleCount(); ++i) {
                record_sample(data[i] / 2147483648.0);
            }
            break;
        }
        case QAudioFormat::Float: {
            const auto* data = buffer.constData<float>();
            for (qsizetype i = 0; i < buffer.sampleCount(); ++i) {
                record_sample(static_cast<double>(data[i]));
            }
            break;
        }
        default:
            break;
        }
    };
    auto matches = [](const QColor& got, int r, int g, int b) {
        return std::abs(got.red() - r) <= 10 && std::abs(got.green() - g) <= 10 && std::abs(got.blue() - b) <= 10;
    };

    QObject::connect(&audio_tap, &QAudioBufferOutput::audioBufferReceived, &loop,
        [&](const QAudioBuffer& buffer) {
            if (buffer.frameCount() <= 0) {
                return;
            }
            ++result.audio_buffers;
            result.audio_frames += buffer.frameCount();
            result.saw_audio = true;
            result.audio_sample_rate = buffer.format().sampleRate();
            result.audio_channels = buffer.format().channelCount();
            consume_samples(buffer);
            if (result.first_audio_start < 0) {
                result.first_audio_start = buffer.startTime();
            }
        });

    QObject::connect(&video_sink, &QVideoSink::videoFrameChanged, &loop, [&](const QVideoFrame& frame) {
        if (!frame.isValid()) {
            return;
        }
        ++result.video_frames;
        result.saw_video = true;
        if (result.video_frames == 1) {
            result.first_video_size = frame.size();
            result.first_video_start = frame.startTime();
            QVideoFrame copy(frame);
            if (copy.map(QVideoFrame::ReadOnly) && copy.planeCount() > 0) {
                const auto* bytes = copy.bits(0);
                const int count = std::min(copy.mappedBytes(0), 4096);
                int lo = 255;
                int hi = 0;
                for (int i = 0; i < count; ++i) {
                    lo = std::min(lo, static_cast<int>(bytes[i]));
                    hi = std::max(hi, static_cast<int>(bytes[i]));
                }
                result.video_byte_variation = hi - lo;
                copy.unmap();
            }
            const QImage image = frame.toImage();
            if (!image.isNull() && image.width() >= 21 && image.height() >= 21) {
                result.video_pattern_matches += matches(image.pixelColor(0, 0), 0, 0, 255) ? 1 : 0;
                result.video_pattern_matches += matches(image.pixelColor(20, 0), 60, 0, 32) ? 1 : 0;
                result.video_pattern_matches += matches(image.pixelColor(0, 20), 0, 100, 32) ? 1 : 0;
                result.video_pattern_matches += matches(image.pixelColor(20, 20), 60, 100, 255) ? 1 : 0;
            }
        }
    });

    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &loop, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            result.eof = true;
            finished = true;
            loop.quit();
        } else if (status == QMediaPlayer::InvalidMedia) {
            result.error = true;
            result.error_string = player.errorString();
            finished = true;
            loop.quit();
        }
    });
    QObject::connect(&player, &QMediaPlayer::errorChanged, &loop, [&] {
        if (player.error() != QMediaPlayer::NoError) {
            result.error = true;
            result.error_string = player.errorString();
            finished = true;
            loop.quit();
        }
    });

    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&] {
        if (!finished) {
            result.error = true;
            result.timed_out = true;
            result.error_string = "decode timeout";
            finished = true;
            loop.quit();
        }
    });

    player.setSource(QUrl::fromLocalFile(path));
    player.play();
    timeout.start(timeout_ms);
    if (!finished) {
        loop.exec();
    }
    player.stop();
    return result;
}

} // namespace c16_l14
