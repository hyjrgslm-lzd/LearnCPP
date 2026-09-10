#include <iostream>

#if defined(__has_include)
#if __has_include(<meta>)
#include <meta>
#define C04_HAS_META_HEADER 1
#endif
#endif

#if defined(C04_FORCE_CONSTEVAL_ONLY_VALUES_PROBE) || (defined(__cpp_consteval) && __cpp_consteval >= 202606L)
#define C04_HAS_CONSTEVAL_ONLY_VALUES 1
#endif
#ifndef C04_HAS_META_HEADER
#define C04_HAS_META_HEADER 0
#endif
#ifndef C04_HAS_CONSTEVAL_ONLY_VALUES
#define C04_HAS_CONSTEVAL_ONLY_VALUES 0
#endif

int main() {
    std::cout << "probe=consteval_only_values header=" << C04_HAS_META_HEADER
              << " macro=" << C04_HAS_CONSTEVAL_ONLY_VALUES
              << " body=" << (C04_HAS_META_HEADER && C04_HAS_CONSTEVAL_ONLY_VALUES) << "\n";
#if !C04_HAS_META_HEADER || !C04_HAS_CONSTEVAL_ONLY_VALUES
    std::cout << "SKIP consteval-only values: missing <meta> or __cpp_consteval DR marker\n";
    return 77;
#elif defined(C04_P4101_NEGATIVE_ESCAPE)
    auto escaped = ^^int;
    (void)escaped;
    std::cout << "FAIL P4101 negative escape unexpectedly compiled\n";
    return 1;
#else
    auto runtime_null = std::meta::info{};
    constexpr std::meta::info int_info = ^^int;
    static_assert(std::meta::info{} != int_info);
    (void)runtime_null;
    std::cout << "PASS P4101R1 null runtime value and non-null constexpr reflection\n";
    return 0;
#endif
}
