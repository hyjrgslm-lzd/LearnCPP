#pragma once

#include <QVideoFrame>

namespace c16_l12 {

struct FrameInfo {
    bool valid = false;
    int width = 0;
    int height = 0;
    int plane_count = 0;
    int bytes_per_line0 = 0;
    int mapped_bytes0 = 0;
    qint64 start_time = -1;
    qint64 end_time = -1;
};

struct Rgb { int r = 0; int g = 0; int b = 0; };

inline FrameInfo inspect_frame(QVideoFrame& frame)
{
    if (!frame.isValid()) return {};
    frame.map(QVideoFrame::ReadOnly);
    return {true, frame.width(), frame.height(), 1, frame.width() * 4, frame.width() * frame.height() * 4,
        frame.startTime(), frame.endTime()};
}

inline Rgb limited_yuv_to_rgb(int y, int u, int v)
{
    return {y + (v - 128), y, y + (u - 128)};
}

} // namespace c16_l12
