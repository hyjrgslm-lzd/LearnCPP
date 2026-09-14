#pragma once
#include <span>

namespace student {
inline double stable_sum(std::span<const double> xs) {
    double sum = 0.0;
    for (double x : xs) sum += x;
    return sum;
}
}
