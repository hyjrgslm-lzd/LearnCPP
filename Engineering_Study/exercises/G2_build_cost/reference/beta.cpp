#include "g2_common/common.hpp"
#include "g2_common/heavy.hpp"

namespace g2_common {
int beta() {
    return header_heavy_value("beta") % 19;
}
}
