#include <bit>
#include <cstdint>
#include <iostream>
#include <version>

int main() {
#if !defined(__cpp_lib_bitops) || __cpp_lib_bitops < 202607L
    std::cout << "SKIP __cpp_lib_bitops is below 202607L; C++29 bit permutation functions not declared\n";
    return 77;
#else
    if (std::bit_reverse(std::uint32_t{0x00001234u}) != 0x24c80000u) {
        std::cerr << "FAIL bit_reverse\n";
        return 1;
    }
    if (std::bit_repeat(std::uint32_t{0xcu}, 4) != 0xccccccccu) {
        std::cerr << "FAIL bit_repeat\n";
        return 1;
    }
    if (std::bit_compress(std::uint32_t{0b1101u}, std::uint32_t{0b0101u}) != 0b11u) {
        std::cerr << "FAIL bit_compress\n";
        return 1;
    }
    if (std::bit_expand(std::uint32_t{0b11u}, std::uint32_t{0b0101u}) != 0b0101u) {
        std::cerr << "FAIL bit_expand\n";
        return 1;
    }

    std::cout << "PASS C++29 bit permutations\n";
    return 0;
#endif
}
