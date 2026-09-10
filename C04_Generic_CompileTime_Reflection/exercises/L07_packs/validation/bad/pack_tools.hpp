#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>

namespace c04 {

template<class... Ts>
inline constexpr auto count_types = sizeof...(Ts);

template<auto... Vs>
inline constexpr auto count_values = sizeof...(Vs);

template<bool... Vs>
inline constexpr bool all_true = (false && ... && Vs);

template<bool... Vs>
inline constexpr bool any_true = (Vs || ...);

template<auto... Vs>
inline constexpr auto sum_values = (0 + ... + Vs);

template<auto... Vs>
inline constexpr auto left_subtract = (0 - ... - Vs);

template<auto... Vs>
inline constexpr auto right_subtract = (Vs - ... - 0);

template<class... Fs>
constexpr void call_in_order(Fs&&... fs) {
    (static_cast<void>(std::forward<Fs>(fs)()), ...);
}

template<auto V>
struct constant {
    static constexpr auto value = V;
};

template<std::size_t N>
struct fixed_string {
    char value[N]{};

    constexpr fixed_string(const char (&text)[N]) {
        std::copy_n(text, N, value);
    }
};

template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template<fixed_string Name, auto V>
struct named_value {
    static constexpr auto name = Name;
    static constexpr auto value = V;
};

template<template<class> class F, class T>
using apply_unary_template = F<T>;

} // namespace c04
