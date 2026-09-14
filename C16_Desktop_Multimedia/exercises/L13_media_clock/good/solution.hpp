#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>

namespace c16_l13 {

enum class Decision { Display, Wait, Drop };
struct VideoFrame { long long pts_us = 0; int generation = 0; };

class ManualClock {
public:
    long long now_us() const noexcept { return now_; }
    void advance_us(long long delta)
    {
        if (delta < 0) throw std::invalid_argument("delta");
        if (delta > std::numeric_limits<long long>::max() - now_) throw std::overflow_error("clock");
        now_ += delta;
    }
private:
    long long now_ = 0;
};

class MediaClock {
public:
    explicit MediaClock(ManualClock& source) : source_(source) {}
    void start(long long media_us)
    {
        if (media_us < 0) throw std::invalid_argument("media");
        wall0_ = source_.now_us();
        media0_ = media_us;
        stopped_ = false;
    }
    void pause() { media0_ = position_us(); stopped_ = true; }
    void resume() { wall0_ = source_.now_us(); stopped_ = false; }
    void set_rate(double r)
    {
        if (!(r > 0.0) || r > 64.0 || !std::isfinite(r)) throw std::invalid_argument("rate");
        media0_ = position_us();
        wall0_ = source_.now_us();
        rate_ = r;
    }
    int seek(long long media_us)
    {
        if (media_us < 0) throw std::invalid_argument("media");
        if (epoch_ == std::numeric_limits<int>::max()) throw std::overflow_error("generation");
        const auto now = source_.now_us();
        media0_ = media_us;
        wall0_ = now;
        return ++epoch_;
    }
    long long position_us() const
    {
        if (stopped_) return media0_;
        if (source_.now_us() < wall0_) throw std::overflow_error("clock");
        const auto scaled = static_cast<long double>(source_.now_us() - wall0_) * static_cast<long double>(rate_);
        if (scaled > static_cast<long double>(std::numeric_limits<long long>::max())) throw std::overflow_error("scale");
        const auto delta = static_cast<long long>(scaled);
        if (delta > std::numeric_limits<long long>::max() - media0_) throw std::overflow_error("media");
        return media0_ + delta;
    }
    Decision classify(VideoFrame frame) const
    {
        if (frame.generation != epoch_) return Decision::Drop;
        const auto ahead = static_cast<long double>(frame.pts_us) - static_cast<long double>(position_us());
        if (ahead > 30000) return Decision::Wait;
        if (ahead < -30000) return Decision::Drop;
        return Decision::Display;
    }
private:
    ManualClock& source_;
    long long wall0_ = 0;
    long long media0_ = 0;
    double rate_ = 1.0;
    bool stopped_ = true;
    int epoch_ = 0;
};

} // namespace c16_l13
