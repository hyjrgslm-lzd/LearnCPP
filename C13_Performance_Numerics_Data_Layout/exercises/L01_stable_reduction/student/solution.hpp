#pragma once
#include "c13/numerics.hpp"

namespace student {
inline double stable_sum(std::span<const double> xs) {
    return c13::naive_sum(xs);
}
}
