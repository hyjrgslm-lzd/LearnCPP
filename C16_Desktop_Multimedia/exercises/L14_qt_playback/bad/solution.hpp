#pragma once

#include <QFileInfo>
#include <QSize>
#include <QString>

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

inline DecodeProbe probe_media(const QString& path, int)
{
    DecodeProbe out;
    out.eof = QFileInfo::exists(path);
    out.error = !out.eof;
    return out;
}

} // namespace c16_l14
