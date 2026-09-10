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

struct decimal_result {
    int value;
    bool digits;
    bool fits;
};

template<fixed_string Text>
consteval decimal_result scan_decimal() {
    decimal_result result{0, sizeof(Text.value) > 1, true};
    for (std::size_t i = 0; i + 1 < sizeof(Text.value); ++i) {
        const char ch = Text.value[i];
        if (ch < '0' || ch > '9') return {0, false, true};
        const int digit = ch - '0';
        if (result.value > (std::numeric_limits<int>::max() - digit) / 10)
            return {0, true, false};
        result.value = result.value * 10 + digit;
    }
    return result;
}

template<fixed_string Text>
consteval int parse_decimal() {
    constexpr auto parsed = scan_decimal<Text>();
    static_assert(parsed.digits, "decimal_value accepts only decimal digits");
    static_assert(parsed.fits, "decimal_value exceeds int range");
    return parsed.value;
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
