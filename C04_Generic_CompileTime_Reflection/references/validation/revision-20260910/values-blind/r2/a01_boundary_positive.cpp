#include <compiletime_values.hpp>

#include <array>
#include <iostream>
#include <string_view>

using namespace c04_values;

inline constexpr auto boundary_rows = std::array{
    make_row("", 0),
    make_row("123456789012345", 15),
    make_row("mid", 3),
};

int main() {
    constexpr auto table = make_table<boundary_rows>();
    static_assert(decltype(table)::size == 3);
    static_assert(find(table, "").value_or(-1) == 0);
    static_assert(find(table, "123456789012345").value_or(-1) == 15);
    static_assert(!find(table, std::string_view{"", 1}).has_value());
    std::cout << "A01 boundary r2 positive probe passed\n";
}
