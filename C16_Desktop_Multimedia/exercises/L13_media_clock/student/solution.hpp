#pragma once

namespace c16_l13 {

enum class Decision { Display, Wait, Drop };
struct VideoFrame { long long pts_us = 0; int generation = 0; };

inline int todo_value() { static volatile int value = 0; return value; }

class ManualClock {
public:
    long long now_us() const noexcept { return now_; }
    void advance_us(long long delta) noexcept { now_ += delta; }
private:
    long long now_ = 0;
};

class MediaClock {
public:
    explicit MediaClock(ManualClock&) {}
    void start(long long) {}
    void pause() {}
    void resume() {}
    void set_rate(double) {}
    int seek(long long) { return todo_value(); }
    long long position_us() const { return todo_value(); }
    Decision classify(VideoFrame) const { return todo_value() == 0 ? Decision::Drop : Decision::Display; }
};

} // namespace c16_l13
