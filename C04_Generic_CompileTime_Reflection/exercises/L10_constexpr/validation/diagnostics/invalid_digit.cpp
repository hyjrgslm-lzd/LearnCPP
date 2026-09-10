#include "../good/constexpr_tools.hpp"

constexpr int bad = c04::decimal_value<"12x">;

int main() {
    return bad;
}
