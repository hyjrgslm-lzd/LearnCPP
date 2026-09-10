#pragma once

#include <cstddef>
#include <initializer_list>
#include <limits>
#include <vector>

namespace c04 {

template<std::size_t N>
struct fixed_string {
    char value[N]{};

    constexpr fixed_string(const char (&text)[N]) {
        for (std::size_t i = 0; i != N; ++i) {
            value[i] = text[i];
        }
    }
};

template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template<fixed_string Text, std::size_t I, int Result>
consteval int parse_decimal_impl() {
    constexpr auto count = sizeof(Text.value);
    if constexpr (count <= 1) {
        static_assert(count > 1, "decimal_value accepts only decimal digits");
        return 0;
    } else if constexpr (I + 1 == count) {
        return Result;
    } else {
        constexpr char ch = Text.value[I];
        if constexpr (ch < '0' || ch > '9') {
            static_assert(ch >= '0' && ch <= '9', "decimal_value accepts only decimal digits");
            return 0;
        } else {
            constexpr int digit = ch - '0';
            constexpr bool fits = Result <= (std::numeric_limits<int>::max() - digit) / 10;
            if constexpr (!fits) {
                static_assert(fits, "decimal_value exceeds int range");
                return 0;
            } else {
                return parse_decimal_impl<Text, I + 1, Result * 10 + digit>();
            }
        }
    }
}

template<fixed_string Text>
consteval int parse_decimal() {
    return parse_decimal_impl<Text, 0, 0>();
}

template<fixed_string Text>
inline constexpr int decimal_value = parse_decimal<Text>();

constexpr int constexpr_vector_sum(std::initializer_list<int> values) {
    std::vector<int> copy(values);
    int sum = 0;
    for (int value : copy) {
        sum += value;
    }
    return sum;
}

constexpr int phase_value(int value) {
    if consteval {
        return value + 1;
    } else {
        return value + 2;
    }
}

} // namespace c04
