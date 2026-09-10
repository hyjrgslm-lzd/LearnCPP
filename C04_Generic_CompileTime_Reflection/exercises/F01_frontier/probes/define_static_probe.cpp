#include <iostream>
#include <string_view>
#include <vector>

#if defined(__has_include)
#if __has_include(<meta>)
#include <meta>
#define C04_HAS_META_HEADER 1
#endif
#endif

#if defined(C04_FORCE_DEFINE_STATIC_PROBE)
#define C04_HAS_DEFINE_STATIC 1
#elif defined(__cpp_lib_define_static) && __cpp_lib_define_static >= 202506L
#define C04_HAS_DEFINE_STATIC 1
#endif
#ifndef C04_HAS_META_HEADER
#define C04_HAS_META_HEADER 0
#endif
#ifndef C04_HAS_DEFINE_STATIC
#define C04_HAS_DEFINE_STATIC 0
#endif

int main() {
    std::cout << "probe=define_static header=" << C04_HAS_META_HEADER
              << " macro=" << C04_HAS_DEFINE_STATIC
              << " body=" << (C04_HAS_META_HEADER && C04_HAS_DEFINE_STATIC) << "\n";
#if !C04_HAS_META_HEADER || !C04_HAS_DEFINE_STATIC
    std::cout << "SKIP define_static: missing <meta> or __cpp_lib_define_static\n";
    return 77;
#else
    constexpr char const* text = std::define_static_string("abc");
    static_assert(std::string_view{text} == "abc");
    constexpr int const* object = std::define_static_object(42);
    static_assert(*object == 42);
    constexpr auto span = std::define_static_array(std::vector{1, 2, 3});
    static_assert(span.size() == 3);
    static_assert(span[2] == 3);
    std::cout << "PASS define_static_string/object/array\n";
    return 0;
#endif
}
