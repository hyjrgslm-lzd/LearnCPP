#include <bit>
#include <cstdint>
#include <iostream>
#include <version>

int main() {
#if !defined(__cpp_lib_bitops) || __cpp_lib_bitops < 202606L
    std::cout << "SKIP __cpp_lib_bitops is below 202606L; C++29 shl/shr not declared\n";
    return 77;
#else
    if (std::shl(std::uint32_t{1}, 31) != 0x80000000u || std::shl(std::uint32_t{1}, 32) != 0u) {
        std::cerr << "FAIL shl overlong behavior\n";
        return 1;
    }
    if (std::shr(std::uint32_t{0x80000000u}, 31) != 1u || std::shr(std::uint32_t{1}, 32) != 0u) {
        std::cerr << "FAIL shr overlong behavior\n";
        return 1;
    }
    if (std::shl(std::uint32_t{8}, -1) != 4u || std::shr(std::uint32_t{8}, -1) != 16u) {
        std::cerr << "FAIL negative shift opposite-direction behavior\n";
        return 1;
    }

    std::cout << "PASS C++29 shl/shr\n";
    return 0;
#endif
}
