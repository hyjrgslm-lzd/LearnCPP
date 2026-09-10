#pragma once
#include <array>
#include <cstddef>

namespace c04_expr {
template<std::size_t N>
struct vec {
    std::array<double, N> data{};
    constexpr double& operator[](std::size_t i) { return data[i]; }
    constexpr const double& operator[](std::size_t i) const { return data[i]; }
};
template<class L, class R> constexpr auto operator+(const L&, const R&) { return vec<3>{}; }
template<class E> constexpr auto operator*(const E&, double) { return vec<3>{}; }
template<class E> constexpr auto operator*(double, const E&) { return vec<3>{}; }
template<class E> constexpr auto reverse(const E&) { return vec<3>{}; }
template<class E> constexpr auto eval(const E&) { return vec<3>{}; }
template<std::size_t N, class E> constexpr void assign(vec<N>& out, const E& e) {
    for (std::size_t i = 0; i < N; ++i) out[i] = e[i];
}
}
