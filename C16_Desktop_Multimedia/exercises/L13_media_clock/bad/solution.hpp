#pragma once

namespace c16_l13 {

enum class Decision { Display, Wait, Drop };
struct VideoFrame { long long pts_us = 0; int generation = 0; };

class ManualClock {
public:
    long long now_us() const noexcept { return now_; }
    void advance_us(long long delta) noexcept { now_ += delta; }
private:
    long long now_ = 0;
};

class MediaClock {
public:
    explicit MediaClock(ManualClock& clock) : clock_(clock) {}
    void start(long long media_us) { media_ = media_us; wall_ = clock_.now_us(); }
    void pause() {}
    void resume() {}
    void set_rate(double rate) { rate_ = rate; }
    int seek(long long media_us) { media_ = media_us; return generation_; }
    long long position_us() const { return media_ + static_cast<long long>((clock_.now_us() - wall_) * rate_); }
    Decision classify(VideoFrame frame) const { return frame.pts_us < position_us() ? Decision::Drop : Decision::Display; }
private:
    ManualClock& clock_;
    long long wall_ = 0;
    long long media_ = 0;
    double rate_ = 1.0;
    int generation_ = 0;
};

} // namespace c16_l13
