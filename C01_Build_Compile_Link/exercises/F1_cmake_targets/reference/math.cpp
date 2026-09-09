#include "f1_math/math.hpp"

namespace {
constexpr int scale = 2;
}

namespace f1_math {
int add_scaled(int left, int right) {
    return (left + right) * scale;
}
}
