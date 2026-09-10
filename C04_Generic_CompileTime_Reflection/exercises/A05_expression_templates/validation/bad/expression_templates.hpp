#pragma once
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace c04_expr {
template<std::size_t N>
struct vec {
    std::array<double, N> data{};
    constexpr double& operator[](std::size_t i) { return data[i]; }
    constexpr const double& operator[](std::size_t i) const { return data[i]; }
};

template<class T> inline constexpr bool is_expr_v = false;
template<std::size_t N> inline constexpr bool is_expr_v<vec<N>> = true;
template<class T> concept expr = is_expr_v<std::remove_cvref_t<T>>;
template<class T> struct expr_size;
template<std::size_t N> struct expr_size<vec<N>> : std::integral_constant<std::size_t, N> {};
template<class T> inline constexpr std::size_t expr_size_v = expr_size<std::remove_cvref_t<T>>::value;

template<class T>
using store_t = std::conditional_t<std::is_lvalue_reference_v<T>, T, std::remove_cvref_t<T>>;
template<class T> constexpr decltype(auto) at(T&& e, std::size_t i) { return std::forward<T>(e)[i]; }

template<expr L, expr R>
struct add_expr {
    store_t<L> left;
    store_t<R> right;
    static constexpr std::size_t size = expr_size_v<L>;
    constexpr double operator[](std::size_t i) const { return at(left, i) + at(right, i); }
};
template<class L, class R> add_expr(L&&, R&&) -> add_expr<L, R>;
template<class L, class R> inline constexpr bool is_expr_v<add_expr<L, R>> = true;
template<class L, class R> struct expr_size<add_expr<L, R>> : std::integral_constant<std::size_t, add_expr<L, R>::size> {};

template<expr E>
struct scale_expr {
    store_t<E> inner;
    double factor{};
    static constexpr std::size_t size = expr_size_v<E>;
    constexpr double operator[](std::size_t i) const { return at(inner, i) * factor; }
};
template<class E> scale_expr(E&&, double) -> scale_expr<E>;
template<class E> inline constexpr bool is_expr_v<scale_expr<E>> = true;
template<class E> struct expr_size<scale_expr<E>> : std::integral_constant<std::size_t, scale_expr<E>::size> {};

template<expr E>
struct reverse_expr {
    store_t<E> inner;
    static constexpr std::size_t size = expr_size_v<E>;
    constexpr double operator[](std::size_t i) const { return at(inner, size - 1 - i); }
};
template<class E> reverse_expr(E&&) -> reverse_expr<E>;
template<class E> inline constexpr bool is_expr_v<reverse_expr<E>> = true;
template<class E> struct expr_size<reverse_expr<E>> : std::integral_constant<std::size_t, reverse_expr<E>::size> {};

template<expr L, expr R>
    requires (expr_size_v<L> == expr_size_v<R>)
constexpr auto operator+(L&& left, R&& right) { return add_expr<L, R>{std::forward<L>(left), std::forward<R>(right)}; }
template<expr E> constexpr auto operator*(E&& e, double factor) { return scale_expr<E>{std::forward<E>(e), factor}; }
template<expr E> constexpr auto operator*(double factor, E&& e) { return scale_expr<E>{std::forward<E>(e), factor}; }
template<expr E> constexpr auto reverse(E&& e) { return reverse_expr<E>{std::forward<E>(e)}; }

template<expr E>
constexpr auto eval(E&& e) {
    vec<expr_size_v<E>> out{};
    for (std::size_t i = 0; i < out.data.size(); ++i) out[i] = at(e, i);
    return out;
}
template<std::size_t N, expr E>
    requires (N == expr_size_v<E>)
constexpr void assign(vec<N>& out, E&& e) {
    for (std::size_t i = 0; i < N; ++i) out[i] = at(e, i);
}
}
