#pragma once

namespace l01_extern {

template<class T>
T twice(T value);

extern template int twice<int>(int);

int use_a(int value);
int use_b(int value);

} // namespace l01_extern
