#pragma once

namespace l01 {

struct meters {};
struct kilometers {};
struct centimeters {};

template<class T, class Unit>
struct quantity {
    using value_type = T;
    using unit_type = Unit;

    T value{};

    constexpr explicit quantity(T v) : value(v) {}
};

template<class Unit>
inline constexpr int unit_scale_v = 1;

template<class Q>
using quantity_value_t = void;

template<class A, class B>
inline constexpr bool same_unit_v = true;

template<class T, class Unit>
constexpr auto to_base(quantity<T, Unit> q) {
    return q.value;
}

template<class A, class B>
constexpr auto add_same_unit(A a, B b) {
    return quantity<int, meters>{static_cast<int>(a.value + b.value)};
}

} // namespace l01
