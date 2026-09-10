#include "../good/constexpr_tools.hpp"

constexpr int good = c04::decimal_value<"120">;
constexpr int zero_padded = c04::decimal_value<"000120">;
constexpr int max_int = c04::decimal_value<"2147483647">;
static_assert(good == 120);
static_assert(zero_padded == 120);
static_assert(max_int == 2147483647);

int main() {
    return 0;
}
