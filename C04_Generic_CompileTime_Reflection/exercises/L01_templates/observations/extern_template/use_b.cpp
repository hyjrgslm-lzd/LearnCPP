#include "twice.hpp"

namespace l01_extern {

int use_b(int value) {
    return twice<int>(value + 1);
}

} // namespace l01_extern
