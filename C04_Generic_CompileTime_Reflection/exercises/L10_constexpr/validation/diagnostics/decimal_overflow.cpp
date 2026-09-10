#include "../good/constexpr_tools.hpp"

constexpr int bad = c04::decimal_value<"2147483648">;

int main() {
    return bad;
}
