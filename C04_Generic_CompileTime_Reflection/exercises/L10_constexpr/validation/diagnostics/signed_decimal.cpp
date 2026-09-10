#include "../good/constexpr_tools.hpp"

constexpr int bad = c04::decimal_value<"-1">;

int main() {
    return bad;
}
