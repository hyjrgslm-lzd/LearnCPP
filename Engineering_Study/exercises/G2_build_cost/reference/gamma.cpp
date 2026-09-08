#include "g2_common/common.hpp"
#include "g2_common/heavy.hpp"

namespace g2_common {
int gamma() {
    return header_heavy_value("gamma") % 23;
}
}
