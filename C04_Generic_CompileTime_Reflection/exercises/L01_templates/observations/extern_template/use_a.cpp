#include "twice.hpp"

namespace l01_extern {

int use_a(int value) {
    return twice<int>(value);
}

} // namespace l01_extern
