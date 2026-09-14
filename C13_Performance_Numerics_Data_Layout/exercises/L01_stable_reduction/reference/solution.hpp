#pragma once
#include "c13/numerics.hpp"

namespace student {
inline double stable_sum(std::span<const double> xs) {
    return c13::neumaier_sum(xs);
}
}
