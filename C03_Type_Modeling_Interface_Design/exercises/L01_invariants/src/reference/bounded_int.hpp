#pragma once

#include <stdexcept>

namespace l01 {

class BoundedInt {
public:
    BoundedInt(int value, int min, int max) : min_(min), max_(max)
    {
        if (min > max) {
            throw std::invalid_argument("min greater than max");
        }
        set(value);
    }

    int value() const noexcept { return value_; }
    int min() const noexcept { return min_; }
    int max() const noexcept { return max_; }

    void set(int value)
    {
        if (value < min_ || value > max_) {
            throw std::out_of_range("value outside range");
        }
        value_ = value;
    }

private:
    int value_ = 0;
    int min_ = 0;
    int max_ = 0;
};

} // namespace l01

