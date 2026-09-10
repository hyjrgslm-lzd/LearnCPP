#include <iostream>
#include <type_traits>
#include <vector>

#if defined(C04_FORCE_TEMPLATE_NAME_PACK_INDEXING_PROBE) || (defined(C04_HAS_TEMPLATE_NAME_PACK_INDEXING) && C04_HAS_TEMPLATE_NAME_PACK_INDEXING)
#define C04_HAS_TEMPLATE_NAME_PACK_INDEXING 1
#endif
#ifndef C04_HAS_TEMPLATE_NAME_PACK_INDEXING
#define C04_HAS_TEMPLATE_NAME_PACK_INDEXING 0
#endif

#if C04_HAS_TEMPLATE_NAME_PACK_INDEXING
template<class T>
struct Box {
    using type = T;
};

template<class T>
struct Wrap {
    using type = T*;
};

template<template<class> class... TT>
struct PickTemplate {
    template<class T>
    using first = TT...[0]<T>;

    template<class T>
    using second = TT...[1]<T>;
};
#endif

int main() {
    std::cout << "probe=c29_template_name_pack_indexing header=na macro="
#if defined(__cpp_pack_indexing)
              << __cpp_pack_indexing
#else
              << 0
#endif
              << " body=" << C04_HAS_TEMPLATE_NAME_PACK_INDEXING << "\n";
#if !C04_HAS_TEMPLATE_NAME_PACK_INDEXING
    std::cout << "SKIP C++29 template-name pack indexing: no explicit implementation marker\n";
    return 77;
#else
    using Pick = PickTemplate<Box, Wrap>;
    static_assert(std::is_same_v<typename Pick::first<int>::type, int>);
    static_assert(std::is_same_v<typename Pick::second<int>::type, int*>);
    std::cout << "PASS C++29 template-name pack indexing\n";
    return 0;
#endif
}
