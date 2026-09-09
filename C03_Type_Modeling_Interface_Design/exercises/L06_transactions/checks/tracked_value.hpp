#pragma once

#include <stdexcept>

namespace l06 {

class TrackedValue {
public:
    explicit TrackedValue(int value = 0) : value_(value) { ++live_; }

    TrackedValue(const TrackedValue& other) : value_(other.value_)
    {
        copy_gate();
        ++live_;
    }

    TrackedValue& operator=(const TrackedValue& other)
    {
        if (this != &other) {
            copy_gate();
            value_ = other.value_;
        }
        return *this;
    }

    TrackedValue(TrackedValue&& other) noexcept : value_(other.value_) { ++live_; }

    TrackedValue& operator=(TrackedValue&& other) noexcept
    {
        value_ = other.value_;
        return *this;
    }

    ~TrackedValue() { --live_; }

    int value() const noexcept { return value_; }

    static void reset_copy_plan() noexcept
    {
        copies_ = 0;
        throw_at_ = -1;
    }

    static void throw_on_copy(int copy_index) noexcept { throw_at_ = copy_index; }
    static int live_count() noexcept { return live_; }

private:
    static void copy_gate()
    {
        ++copies_;
        if (copies_ == throw_at_) {
            throw std::runtime_error("planned copy failure");
        }
    }

    int value_ = 0;
    inline static int copies_ = 0;
    inline static int throw_at_ = -1;
    inline static int live_ = 0;
};

} // namespace l06

