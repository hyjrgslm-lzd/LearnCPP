#include <compiletime_values.hpp>

#include <array>
#include <iostream>

using namespace c04_values;

inline constexpr auto accepted_16 = make_row("1234567890123456", 16);
inline constexpr auto accepted_non_ascii = make_row("\xC3\xA9", 1);
inline constexpr auto raw = std::array{
    accepted_16,
    accepted_non_ascii,
};
inline constexpr auto lookup_table = make_table<raw>();

int main() {
    const bool accepts_16 = find(lookup_table, "1234567890123456").value_or(-1) == 16;
    const bool accepts_non_ascii = find(lookup_table, "\xC3\xA9").value_or(-1) == 1;
    if (!accepts_16 || !accepts_non_ascii) {
        return 1;
    }
    std::cout << "A01 boundary gap reproduced: implementation accepts 16-char and non-ASCII keys\n";
}
