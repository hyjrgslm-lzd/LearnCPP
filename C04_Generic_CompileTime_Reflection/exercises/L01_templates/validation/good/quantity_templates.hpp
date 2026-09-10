#pragma once

#include <type_traits>

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
inline constexpr int unit_scale_v = 0;

template<>
inline constexpr int unit_scale_v<centimeters> = 1;

template<>
inline constexpr int unit_scale_v<meters> = 100;

template<>
inline constexpr int unit_scale_v<kilometers> = 100000;

template<class Q>
using quantity_value_t = typename Q::value_type;

template<class A, class B>
inline constexpr bool same_unit_v = false;

template<class T, class U, class Unit>
inline constexpr bool same_unit_v<quantity<T, Unit>, quantity<U, Unit>> = true;

template<class T, class Unit>
constexpr auto to_base(quantity<T, Unit> q) {
    return q.value * unit_scale_v<Unit>;
}

template<class A, class B>
    requires same_unit_v<A, B>
constexpr auto add_same_unit(A a, B b) {
    return quantity<decltype(a.value + b.value), typename A::unit_type>{a.value + b.value};
}

} // namespace l01
