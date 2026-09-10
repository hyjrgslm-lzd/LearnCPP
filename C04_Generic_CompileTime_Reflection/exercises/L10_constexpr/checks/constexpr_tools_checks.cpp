#include <check.hpp>
#include <constexpr_tools.hpp>

#include <iostream>

int main() {
    constexpr int parsed = c04::decimal_value<"123">;
    constexpr int zero = c04::decimal_value<"0">;
    constexpr int leading_zero = c04::decimal_value<"00042">;
    constexpr int int_max = c04::decimal_value<"2147483647">;
    constexpr int compile_time_sum = c04::constexpr_vector_sum({1, 2, 3});
    constexpr int compile_time_phase = c04::phase_value(10);

    check(parsed == 123, "decimal_value parses digits at compile time");
    check(zero == 0, "decimal_value handles zero");
    check(leading_zero == 42, "decimal_value allows leading zeros");
    check(int_max == 2147483647, "decimal_value accepts INT_MAX");
    check(compile_time_sum == 6, "constexpr_vector_sum is usable during constant evaluation");
    check(compile_time_phase == 11, "phase_value constant-evaluation branch adds one");
    check(c04::constexpr_vector_sum({4, 5, 6}) == 15, "constexpr_vector_sum can use temporary vector storage");
    int runtime = 10;
    check(c04::phase_value(runtime) == 12, "if consteval branch must distinguish runtime calls");
    std::cout << "L10_constexpr checks OK\n";
}
