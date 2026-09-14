#pragma once

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

class MappedFrame {
public:
    explicit MappedFrame(QVideoFrame& frame) : frame_(frame), mapped_(frame_.map(QVideoFrame::ReadOnly)) {}
    ~MappedFrame() { if (mapped_) frame_.unmap(); }
    bool mapped() const { return mapped_; }
private:
    QVideoFrame& frame_;
    bool mapped_ = false;
};

inline FrameInfo inspect_frame(QVideoFrame& frame)
{
    FrameInfo info;
    if (!frame.isValid()) {
        return info;
    }
    MappedFrame guard(frame);
    if (!guard.mapped()) {
        return info;
    }
    info.valid = true;
    info.width = frame.width();
    info.height = frame.height();
    info.plane_count = frame.planeCount();
    info.bytes_per_line0 = info.plane_count ? frame.bytesPerLine(0) : 0;
    info.mapped_bytes0 = info.plane_count ? frame.mappedBytes(0) : 0;
    info.start_time = frame.startTime();
    info.end_time = frame.endTime();
    return info;
}

inline Rgb limited_yuv_to_rgb(int y, int u, int v)
{
    const int yy = y - 16;
    const int uu = u - 128;
    const int vv = v - 128;
    auto sat = [](int x) { return std::clamp(x, 0, 255); };
    return {
        sat((298 * yy + 409 * vv + 128) / 256),
        sat((298 * yy - 100 * uu - 208 * vv + 128) / 256),
        sat((298 * yy + 516 * uu + 128) / 256),
    };
}

} // namespace c16_l12
