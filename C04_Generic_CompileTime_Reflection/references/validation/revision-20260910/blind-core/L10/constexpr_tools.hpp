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

template<fixed_string Text>
consteval int parse_decimal() {
    constexpr std::size_t length = sizeof(Text.value);
    static_assert(length > 1, "decimal_value requires a non-empty decimal string");

    int result = 0;
    for (std::size_t i = 0; i + 1 < length; ++i) {
        const char ch = Text.value[i];
        static_assert(ch >= '0' && ch <= '9', "decimal_value accepts only ASCII decimal digits");
        const int digit = ch - '0';
        static_assert(result <= (std::numeric_limits<int>::max() - digit) / 10, "decimal_value exceeds int range");
        result = result * 10 + digit;
    }
    return result;
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
