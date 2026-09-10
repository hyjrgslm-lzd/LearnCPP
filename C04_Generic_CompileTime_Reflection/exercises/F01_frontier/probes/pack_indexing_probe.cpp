#include <iostream>
#include <type_traits>

#if (defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L) || defined(C04_FORCE_PACK_INDEXING_PROBE)
#define C04_HAS_PACK_INDEXING 1
#endif
#ifndef C04_HAS_PACK_INDEXING
#define C04_HAS_PACK_INDEXING 0
#endif

#if C04_HAS_PACK_INDEXING
template<int... Values>
consteval int second_value() {
    return Values...[1];
}

template<class... Ts>
using second_type = Ts...[1];
#endif

int main() {
    std::cout << "probe=pack_indexing header=na macro=" << C04_HAS_PACK_INDEXING
              << " body=" << C04_HAS_PACK_INDEXING << "\n";
#if !C04_HAS_PACK_INDEXING
    std::cout << "SKIP pack indexing: __cpp_pack_indexing < 202311L for C++26 value/type packs\n";
    return 77;
#else
    static_assert(second_value<4, 5, 6>() == 5);
    static_assert(std::is_same_v<second_type<char, long, int>, long>);
    std::cout << "PASS C++26 type and value pack indexing\n";
    return 0;
#endif
}
