#include "../good/constexpr_tools.hpp"

constexpr int bad = c04::decimal_value<"">;

int main() {
    return bad;
}
