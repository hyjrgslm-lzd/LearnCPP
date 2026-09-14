#pragma once

#include <QSize>
#include <QVideoFrame>

#include <algorithm>

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
    FrameInfo info;
    if (!frame.isValid() || !frame.map(QVideoFrame::ReadOnly)) {
        return info;
    }
    info.valid = true;
    info.width = frame.size().width();
    info.height = frame.size().height();
    info.plane_count = frame.planeCount();
    if (info.plane_count > 0) {
        info.bytes_per_line0 = frame.bytesPerLine(0);
        info.mapped_bytes0 = frame.mappedBytes(0);
        (void)frame.bits(0);
    }
    info.start_time = frame.startTime();
    info.end_time = frame.endTime();
    frame.unmap();
    return info;
}

inline int clamp8(int value) { return std::clamp(value, 0, 255); }

inline Rgb limited_yuv_to_rgb(int y, int u, int v)
{
    const int c = y - 16;
    const int d = u - 128;
    const int e = v - 128;
    return {
        clamp8((298 * c + 409 * e + 128) >> 8),
        clamp8((298 * c - 100 * d - 208 * e + 128) >> 8),
        clamp8((298 * c + 516 * d + 128) >> 8),
    };
}

} // namespace c16_l12
