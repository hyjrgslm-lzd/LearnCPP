#include "g2_common/common.hpp"
#include "g2_common/heavy.hpp"

namespace g2_common {
int alpha() {
    return header_heavy_value("alpha") % 17;
}
}
