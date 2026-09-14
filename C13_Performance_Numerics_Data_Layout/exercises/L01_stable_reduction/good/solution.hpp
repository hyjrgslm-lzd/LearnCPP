#pragma once
#include <cmath>
#include <span>

namespace student {
inline double stable_sum(std::span<const double> xs) {
    double total = 0.0;
    double lost = 0.0;
    for (double value : xs) {
        const double next = total + value;
        lost += (std::abs(total) >= std::abs(value)) ? (total - next) + value : (value - next) + total;
        total = next;
    }
    return total + lost;
}
}
