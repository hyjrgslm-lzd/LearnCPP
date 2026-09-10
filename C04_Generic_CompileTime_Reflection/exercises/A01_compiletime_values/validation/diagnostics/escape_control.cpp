#include <compiletime_values.hpp>

#include <array>

inline constexpr auto raw = std::array{c04_values::make_row("hp", 1)};
inline constexpr auto table = c04_values::make_table<raw>();

int main() {
    return c04_values::find(table, "hp").value_or(0) == 1 ? 0 : 1;
}
