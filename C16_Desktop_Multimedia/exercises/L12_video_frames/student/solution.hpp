#pragma once

#include <QVideoFrame>

#include <chrono>

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

inline int todo_value()
{
    return std::chrono::steady_clock::now().time_since_epoch().count() == -1 ? 1 : 0;
}
inline FrameInfo inspect_frame(QVideoFrame&)
{
    FrameInfo info;
    info.valid = todo_value() != 0;
    return info;
}
inline Rgb limited_yuv_to_rgb(int, int, int) { return {todo_value(), 0, 0}; }

} // namespace c16_l12
