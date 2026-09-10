#pragma once
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace c04_expr {
template<std::size_t N> struct vec {
    std::array<double, N> data{};
    constexpr double& operator[](std::size_t i) { return data[i]; }
    constexpr const double& operator[](std::size_t i) const { return data[i]; }
};
template<class T> inline constexpr bool is_expr_v = false;
template<std::size_t N> inline constexpr bool is_expr_v<vec<N>> = true;
template<class T> concept expr = is_expr_v<std::remove_cvref_t<T>>;
template<class T> struct expr_size;
template<std::size_t N> struct expr_size<vec<N>> : std::integral_constant<std::size_t, N> {};
template<class T> inline constexpr auto expr_size_v = expr_size<std::remove_cvref_t<T>>::value;
template<class T> using stored = std::conditional_t<std::is_lvalue_reference_v<T>, T, std::remove_cvref_t<T>>;
template<class T> constexpr decltype(auto) elem(T&& value, std::size_t i) { return std::forward<T>(value)[i]; }
template<expr L, expr R> struct sum {
    stored<L> l; stored<R> r; static constexpr auto size = expr_size_v<L>;
    constexpr double operator[](std::size_t i) const { return elem(l, i) + elem(r, i); }
};
template<class L, class R> sum(L&&, R&&) -> sum<L, R>;
template<class L, class R> inline constexpr bool is_expr_v<sum<L, R>> = true;
template<class L, class R> struct expr_size<sum<L, R>> : std::integral_constant<std::size_t, sum<L, R>::size> {};
template<expr E> struct scaled {
    stored<E> e; double k{}; static constexpr auto size = expr_size_v<E>;
    constexpr double operator[](std::size_t i) const { return elem(e, i) * k; }
};
template<class E> scaled(E&&, double) -> scaled<E>;
template<class E> inline constexpr bool is_expr_v<scaled<E>> = true;
template<class E> struct expr_size<scaled<E>> : std::integral_constant<std::size_t, scaled<E>::size> {};
template<expr E> struct reversed {
    stored<E> e; static constexpr auto size = expr_size_v<E>;
    constexpr double operator[](std::size_t i) const { return elem(e, size - 1 - i); }
};
template<class E> reversed(E&&) -> reversed<E>;
template<class E> inline constexpr bool is_expr_v<reversed<E>> = true;
template<class E> struct expr_size<reversed<E>> : std::integral_constant<std::size_t, reversed<E>::size> {};
template<expr L, expr R> requires (expr_size_v<L> == expr_size_v<R>)
constexpr auto operator+(L&& l, R&& r) { return sum<L, R>{std::forward<L>(l), std::forward<R>(r)}; }
template<expr E> constexpr auto operator*(E&& e, double k) { return scaled<E>{std::forward<E>(e), k}; }
template<expr E> constexpr auto operator*(double k, E&& e) { return scaled<E>{std::forward<E>(e), k}; }
template<expr E> constexpr auto reverse(E&& e) { return reversed<E>{std::forward<E>(e)}; }
template<expr E> constexpr auto eval(E&& e) {
    vec<expr_size_v<E>> out{};
    for (std::size_t i = 0; i < out.data.size(); ++i) out[i] = elem(e, i);
    return out;
}
template<std::size_t N, expr E> requires (N == expr_size_v<E>)
constexpr void assign(vec<N>& out, E&& e) { out = eval(std::forward<E>(e)); }
}
