#pragma once

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace my::ranges {

template<class T>
inline constexpr bool enable_view = std::ranges::view<T>;

template<class T>
inline constexpr bool enable_borrowed_range = std::ranges::borrowed_range<T>;

struct begin_fn {
    template<class R>
    constexpr decltype(auto) operator()(R&& r) const noexcept(noexcept(std::ranges::begin(std::forward<R>(r)))) {
        return std::ranges::begin(std::forward<R>(r));
    }
};

struct end_fn {
    template<class R>
    constexpr decltype(auto) operator()(R&& r) const noexcept(noexcept(std::ranges::end(std::forward<R>(r)))) {
        return std::ranges::end(std::forward<R>(r));
    }
};

struct size_fn {
    template<class R>
    constexpr decltype(auto) operator()(R&& r) const noexcept(noexcept(std::ranges::size(std::forward<R>(r)))) {
        return std::ranges::size(std::forward<R>(r));
    }
};

struct iter_move_fn {
    template<class I>
    constexpr decltype(auto) operator()(I&& it) const noexcept(noexcept(std::ranges::iter_move(std::forward<I>(it)))) {
        return std::ranges::iter_move(std::forward<I>(it));
    }
};

inline constexpr begin_fn begin{};
inline constexpr end_fn end{};
inline constexpr size_fn size{};
inline constexpr iter_move_fn iter_move{};

template<class R>
using iterator_t = decltype(my::ranges::begin(std::declval<R&>()));

template<class R>
using sentinel_t = decltype(my::ranges::end(std::declval<R&>()));

template<class R>
using range_reference_t = std::iter_reference_t<iterator_t<R>>;

} // namespace my::ranges
