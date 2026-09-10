#include <array>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace blind_a05 {
template<std::size_t N>
struct vec {
    std::array<double, N> data{};

    constexpr double& operator[](std::size_t i) { return data[i]; }
    constexpr const double& operator[](std::size_t i) const { return data[i]; }
};

template<class T>
struct expr_size;

template<std::size_t N>
struct expr_size<vec<N>> : std::integral_constant<std::size_t, N> {};

template<class T>
inline constexpr std::size_t expr_size_v = expr_size<std::remove_cvref_t<T>>::value;

template<class T>
concept expr = requires(const std::remove_reference_t<T>& value, std::size_t i) {
    { value[i] } -> std::convertible_to<double>;
    expr_size_v<T>;
};

template<class T>
using store_t = std::conditional_t<std::is_lvalue_reference_v<T>, T, std::remove_cvref_t<T>>;

template<class T>
constexpr double at(T&& value, std::size_t i)
{
    return std::forward<T>(value)[i];
}

template<class L, class R>
struct add_expr {
    store_t<L> left;
    store_t<R> right;
    static constexpr std::size_t size = expr_size_v<L>;

    constexpr double operator[](std::size_t i) const
    {
        return at(left, i) + at(right, i);
    }
};

template<class L, class R>
struct expr_size<add_expr<L, R>> : std::integral_constant<std::size_t, add_expr<L, R>::size> {};

template<class E>
struct scale_expr {
    store_t<E> inner;
    double factor{};
    static constexpr std::size_t size = expr_size_v<E>;

    constexpr double operator[](std::size_t i) const
    {
        return at(inner, i) * factor;
    }
};

template<class E>
struct expr_size<scale_expr<E>> : std::integral_constant<std::size_t, scale_expr<E>::size> {};

template<class E>
struct reverse_expr {
    store_t<E> inner;
    static constexpr std::size_t size = expr_size_v<E>;

    constexpr double operator[](std::size_t i) const
    {
        return at(inner, size - 1 - i);
    }
};

template<class E>
struct expr_size<reverse_expr<E>> : std::integral_constant<std::size_t, reverse_expr<E>::size> {};

template<expr L, expr R>
    requires (expr_size_v<L> == expr_size_v<R>)
constexpr auto operator+(L&& left, R&& right)
{
    return add_expr<L, R>{std::forward<L>(left), std::forward<R>(right)};
}

template<expr E>
constexpr auto operator*(E&& inner, double factor)
{
    return scale_expr<E>{std::forward<E>(inner), factor};
}

template<expr E>
constexpr auto operator*(double factor, E&& inner)
{
    return std::forward<E>(inner) * factor;
}

template<expr E>
constexpr auto reverse(E&& inner)
{
    return reverse_expr<E>{std::forward<E>(inner)};
}

template<expr E>
constexpr auto eval(E&& expression)
{
    vec<expr_size_v<E>> out{};
    for (std::size_t i = 0; i < out.data.size(); ++i) {
        out[i] = at(expression, i);
    }
    return out;
}

template<std::size_t N, expr E>
    requires (N == expr_size_v<E>)
constexpr void assign(vec<N>& out, E&& expression)
{
    out = eval(std::forward<E>(expression));
}
}

template<std::size_t N>
constexpr bool same(const blind_a05::vec<N>& left, const blind_a05::vec<N>& right)
{
    for (std::size_t i = 0; i < N; ++i) {
        if (left[i] != right[i]) {
            return false;
        }
    }
    return true;
}

template<class L, class R>
concept addable = requires(L left, R right) {
    left + right;
};

int main()
{
    using blind_a05::eval;
    using blind_a05::reverse;
    using blind_a05::vec;

    vec<5> a{{-1.0, 0.5, 7.0, 8.0, -3.0}};
    vec<5> b{{2.0, -4.0, 0.25, 9.0, 11.0}};
    auto expression = reverse(vec<5>{{10.0, 20.0, 30.0, 40.0, 50.0}}) + (a + 3.0 * b);
    assert(same(eval(expression), vec<5>{{55.0, 28.5, 37.75, 75.0, 40.0}}));

    auto borrowed = a + b;
    b[0] = 100.0;
    assert(same(eval(borrowed), vec<5>{{99.0, -3.5, 7.25, 17.0, 8.0}}));

    auto owned = eval(vec<5>{{1.0, 1.0, 1.0, 1.0, 1.0}} + b);
    b[1] = 200.0;
    assert(same(owned, vec<5>{{101.0, -3.0, 1.25, 10.0, 12.0}}));

    vec<4> alias{{1.0, 2.0, 3.0, 4.0}};
    blind_a05::assign(alias, reverse(alias));
    assert(same(alias, vec<4>{{4.0, 3.0, 2.0, 1.0}}));

    static_assert(!addable<vec<2>, vec<3>>);
}
