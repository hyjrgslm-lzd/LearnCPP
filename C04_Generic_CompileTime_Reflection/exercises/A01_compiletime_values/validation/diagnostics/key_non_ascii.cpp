#include <compiletime_values.hpp>

using namespace c04_values;

inline constexpr auto non_ascii = make_row("\xC3\xA9", 1);

int main() {
    return non_ascii.value;
}
