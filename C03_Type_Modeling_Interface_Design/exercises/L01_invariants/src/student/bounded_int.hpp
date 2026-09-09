#pragma once

namespace l01 {

class BoundedInt {
public:
    BoundedInt(int value, int min, int max) : value_(value), min_(min), max_(max) {}
    int value() const noexcept { return value_; }
    int min() const noexcept { return min_; }
    int max() const noexcept { return max_; }
    void set(int value) noexcept { value_ = value; }

private:
    int value_ = 0;
    int min_ = 0;
    int max_ = 0;
};

} // namespace l01

