#include <iostream>

#if defined(__has_include)
#if __has_include(<meta>)
#include <meta>
#define C04_HAS_META_HEADER 1
#endif
#endif

#if defined(C04_FORCE_ATTRIBUTES_REFLECTION_PROBE) || defined(__cpp_impl_reflection_attributes)
#define C04_HAS_ATTRIBUTES_REFLECTION 1
#endif
#ifndef C04_HAS_META_HEADER
#define C04_HAS_META_HEADER 0
#endif
#ifndef C04_HAS_ATTRIBUTES_REFLECTION
#define C04_HAS_ATTRIBUTES_REFLECTION 0
#endif

#if C04_HAS_ATTRIBUTES_REFLECTION
struct [[deprecated]] DeprecatedType {};

consteval bool attributes_work() {
    auto attributes = std::meta::attributes_of(^^DeprecatedType);
    return attributes.size() == 1
        && std::meta::is_attribute(attributes[0])
        && std::meta::has_attribute(^^DeprecatedType, ^^[[deprecated]])
        && std::meta::identifier_of(^^[[deprecated]]) == "deprecated";
}
#else
struct DeprecatedType {};
#endif

int main() {
    std::cout << "probe=attributes_reflection header=" << C04_HAS_META_HEADER
              << " macro=" << C04_HAS_ATTRIBUTES_REFLECTION
              << " body=" << (C04_HAS_META_HEADER && C04_HAS_ATTRIBUTES_REFLECTION) << "\n";
#if !C04_HAS_META_HEADER || !C04_HAS_ATTRIBUTES_REFLECTION
    std::cout << "SKIP attributes reflection: P3385R8 is a proposal and no implementation marker is set\n";
    return 77;
#else
    static_assert(attributes_work());
    std::cout << "PASS attributes reflection proposal probe\n";
    return 0;
#endif
}
