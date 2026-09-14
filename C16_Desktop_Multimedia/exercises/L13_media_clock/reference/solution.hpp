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
        if (delta < 0) {
            throw std::invalid_argument("clock delta must be non-negative");
        }
        now_ = checked_add(now_, delta);
    }
private:
    static long long checked_add(long long a, long long b)
    {
        if (b > std::numeric_limits<long long>::max() - a) {
            throw std::overflow_error("clock overflow");
        }
        return a + b;
    }

    long long now_ = 0;
};

class MediaClock {
public:
    explicit MediaClock(ManualClock& clock) : clock_(clock) {}

    void start(long long media_us)
    {
        validate_media(media_us);
        anchor_wall_us_ = clock_.now_us();
        anchor_media_us_ = media_us;
        paused_ = false;
    }

    void pause()
    {
        if (!paused_) {
            anchor_media_us_ = position_us();
            paused_ = true;
        }
    }

    void resume()
    {
        if (paused_) {
            anchor_wall_us_ = clock_.now_us();
            paused_ = false;
        }
    }

    void set_rate(double rate)
    {
        if (rate <= 0.0 || rate > max_rate_ || !std::isfinite(rate)) {
            throw std::invalid_argument("invalid playback rate");
        }
        const auto settled = position_us();
        const auto now = clock_.now_us();
        anchor_media_us_ = settled;
        anchor_wall_us_ = now;
        rate_ = rate;
    }

    int seek(long long media_us)
    {
        validate_media(media_us);
        if (generation_ == std::numeric_limits<int>::max()) {
            throw std::overflow_error("generation overflow");
        }
        const auto now = clock_.now_us();
        anchor_media_us_ = media_us;
        anchor_wall_us_ = now;
        return ++generation_;
    }

    long long position_us() const
    {
        if (paused_) {
            return anchor_media_us_;
        }
        const auto wall_delta = checked_subtract_nonnegative(clock_.now_us(), anchor_wall_us_);
        const auto scaled = static_cast<long double>(wall_delta) * static_cast<long double>(rate_);
        if (scaled > static_cast<long double>(std::numeric_limits<long long>::max())) {
            throw std::overflow_error("media position overflow");
        }
        return checked_add(anchor_media_us_, static_cast<long long>(scaled));
    }

    Decision classify(VideoFrame frame) const
    {
        if (frame.generation != generation_) {
            return Decision::Drop;
        }
        const auto delta = static_cast<long double>(frame.pts_us) - static_cast<long double>(position_us());
        if (delta > display_tolerance_us_) {
            return Decision::Wait;
        }
        if (delta < -drop_tolerance_us_) {
            return Decision::Drop;
        }
        return Decision::Display;
    }

private:
    static void validate_media(long long media_us)
    {
        if (media_us < 0) {
            throw std::invalid_argument("media time must be non-negative");
        }
    }

    static long long checked_subtract_nonnegative(long long a, long long b)
    {
        if (a < b) {
            throw std::overflow_error("clock moved backwards");
        }
        return a - b;
    }

    static long long checked_add(long long a, long long b)
    {
        if (b > std::numeric_limits<long long>::max() - a) {
            throw std::overflow_error("media time overflow");
        }
        return a + b;
    }

    ManualClock& clock_;
    long long anchor_wall_us_ = 0;
    long long anchor_media_us_ = 0;
    double rate_ = 1.0;
    bool paused_ = true;
    int generation_ = 0;
    static constexpr double max_rate_ = 64.0;
    long long display_tolerance_us_ = 30000;
    long long drop_tolerance_us_ = 30000;
};

} // namespace c16_l13
