#include "../good/constexpr_tools.hpp"

constexpr int bad = c04::decimal_value<"12\0x">;

int main() {
    return bad;
}
