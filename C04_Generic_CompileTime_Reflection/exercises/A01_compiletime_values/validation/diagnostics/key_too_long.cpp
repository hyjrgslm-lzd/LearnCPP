#include <compiletime_values.hpp>

using namespace c04_values;

inline constexpr auto too_long = make_row("1234567890123456", 16);

int main() {
    return too_long.value;
}
