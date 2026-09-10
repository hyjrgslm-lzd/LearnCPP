#include <compiletime_values.hpp>

#include <array>

using namespace c04_values;

inline constexpr auto boundary = std::array{
    make_row("", 0),
    make_row("ascii", 1),
    make_row("123456789012345", 15),
};

int main() {
    constexpr auto table = make_table<boundary>();
    return decltype(table)::size == 3 ? 0 : 1;
}
