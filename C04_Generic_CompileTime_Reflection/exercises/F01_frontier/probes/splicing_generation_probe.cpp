#include <iostream>

#if defined(__has_include)
#if __has_include(<meta>)
#include <meta>
#define C04_HAS_META_HEADER 1
#endif
#endif

#if defined(C04_FORCE_SPLICING_PROBE) || (defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202603L)
#define C04_HAS_SPLICING_SYNTAX 1
#endif
#ifndef C04_HAS_META_HEADER
#define C04_HAS_META_HEADER 0
#endif
#ifndef C04_HAS_SPLICING_SYNTAX
#define C04_HAS_SPLICING_SYNTAX 0
#endif

struct SpliceRecord {
    int id;
};

int main() {
    std::cout << "probe=splicing_generation header=" << C04_HAS_META_HEADER
              << " macro=" << C04_HAS_SPLICING_SYNTAX
              << " body=" << (C04_HAS_META_HEADER && C04_HAS_SPLICING_SYNTAX) << "\n";
#if !C04_HAS_META_HEADER || !C04_HAS_SPLICING_SYNTAX
    std::cout << "SKIP splicing: missing <meta> or explicit splicing syntax macro\n";
    return 77;
#else
    constexpr std::meta::info id = ^^SpliceRecord::id;
    SpliceRecord value{3};
    value.[:id:] = 9;
    if (value.id != 9) {
        std::cout << "FAIL spliced field write\n";
        return 1;
    }
    std::cout << "PASS member splicing read/write\n";
    return 0;
#endif
}
