#pragma once

#include <cstddef>
#include <initializer_list>

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
    int result = 0;
    for (char ch : Text.value) {
        if (ch == '\0') {
            return result;
        }
        result = result * 10 + (ch - '0');
    }
    return result;
}

template<fixed_string Text>
inline constexpr int decimal_value = parse_decimal<Text>();

constexpr int constexpr_vector_sum(std::initializer_list<int> values) {
    int sum = 0;
    for (int value : values) {
        sum += value;
    }
    return sum;
}
constexpr int phase_value(int value) {
    return value + 1;
}
} // namespace c04
