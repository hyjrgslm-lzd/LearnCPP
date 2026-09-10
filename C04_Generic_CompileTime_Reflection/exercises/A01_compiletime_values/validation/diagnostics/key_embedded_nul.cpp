#include <compiletime_values.hpp>

using namespace c04_values;

inline constexpr auto embedded_nul = make_row("a\0b", 1);

int main() {
    return embedded_nul.value;
}
