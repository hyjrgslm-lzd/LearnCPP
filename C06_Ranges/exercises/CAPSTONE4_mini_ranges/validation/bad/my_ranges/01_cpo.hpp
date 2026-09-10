#pragma once

#include <iterator>
#include <type_traits>
#include <utility>

namespace my::ranges {

template<class T>
inline constexpr bool enable_view = false;
template<class T>
inline constexpr bool enable_borrowed_range = false;

namespace _begin {
struct fn {
    template<class T, std::size_t N>
    constexpr T* operator()(T (&a)[N]) const noexcept { return a; }
    template<class R>
        requires requires(R& r) { r.begin(); }
    constexpr auto operator()(R& r) const { return r.begin(); }
    template<class R>
        requires (!requires(R& r) { r.begin(); }) && requires(R& r) { begin(r); }
    constexpr auto operator()(R& r) const { return begin(r); }
};
}
inline constexpr _begin::fn begin{};

namespace _end {
struct fn {
    template<class T, std::size_t N>
    constexpr T* operator()(T (&a)[N]) const noexcept { return a + N; }
    template<class R>
        requires requires(R& r) { r.end(); }
    constexpr auto operator()(R& r) const { return r.end(); }
    template<class R>
        requires (!requires(R& r) { r.end(); }) && requires(R& r) { end(r); }
    constexpr auto operator()(R& r) const { return end(r); }
};
}
inline constexpr _end::fn end{};

template<class R>
using iterator_t = decltype(my::ranges::begin(std::declval<R&>()));
template<class R>
using sentinel_t = decltype(my::ranges::end(std::declval<R&>()));
template<class R>
using range_reference_t = std::iter_reference_t<iterator_t<R>>;

namespace _iter_move {
struct fn {
    template<class I>
    constexpr decltype(auto) operator()(I&& it) const {
        if constexpr (requires { iter_move(std::forward<I>(it)); }) {
            return iter_move(std::forward<I>(it));
        } else {
            return std::move(*std::forward<I>(it));
        }
    }
};
}
inline constexpr _iter_move::fn iter_move{};

namespace _size {
struct fn {
    template<class R>
        requires requires(R& r) { r.size(); }
    constexpr auto operator()(R& r) const { return r.size(); }
    template<class R>
        requires (!requires(R& r) { r.size(); }) &&
                 requires(R& r) { my::ranges::end(r) - my::ranges::begin(r); }
    constexpr auto operator()(R& r) const { return my::ranges::end(r) - my::ranges::begin(r); }
};
}
inline constexpr _size::fn size{};

} // namespace my::ranges
