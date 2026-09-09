#include "g2_common/common.hpp"
#include "g2_common/heavy.hpp"

namespace g2_common {
int answer() {
    return alpha() + beta() + gamma() + (header_heavy_value("common") % 11) + 17;
}
}
