#include "twice.hpp"

namespace l01_extern {

template<class T>
T twice(T value) {
    return value + value;
}

template int twice<int>(int);

} // namespace l01_extern
