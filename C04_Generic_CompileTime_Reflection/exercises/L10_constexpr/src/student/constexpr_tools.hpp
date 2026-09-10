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
inline constexpr int decimal_value = 0;

constexpr int constexpr_vector_sum(std::initializer_list<int>) {
    return 0;
}

constexpr int phase_value(int value) {
    return value;
}

} // namespace c04
