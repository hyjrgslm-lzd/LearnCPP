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

template<fixed_string Text, std::size_t Index, int Accumulator, bool Done = (Index + 1 == sizeof(Text.value))>
struct decimal_parser;

template<fixed_string Text, std::size_t Index, int Accumulator>
struct decimal_parser<Text, Index, Accumulator, true> {
    static constexpr int value = Accumulator;
};

template<fixed_string Text, std::size_t Index, int Accumulator>
struct decimal_parser<Text, Index, Accumulator, false> {
    static constexpr char ch = Text.value[Index];
    static constexpr int value = [] {
        if constexpr (ch < '0' || ch > '9') {
            static_assert(ch >= '0' && ch <= '9', "decimal_value accepts only decimal digits");
            return 0;
        } else {
            constexpr int digit = ch - '0';
            constexpr bool fits = Accumulator <= (std::numeric_limits<int>::max() - digit) / 10;
            if constexpr (fits) {
                return decimal_parser<Text, Index + 1, Accumulator * 10 + digit>::value;
            } else {
                static_assert(fits, "decimal_value exceeds int range");
                return 0;
            }
        }
    }();
};

template<fixed_string Text>
struct decimal_parse {
    static_assert(sizeof(Text.value) > 1, "decimal_value accepts only decimal digits");
    static constexpr int value = decimal_parser<Text, 0, 0>::value;
};

template<fixed_string Text>
consteval int parse_decimal() {
    return decimal_parse<Text>::value;
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
