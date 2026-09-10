#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

namespace c04_overload {

struct member_result {
    int calls{};
};

template<std::size_t N>
struct literal_result {
    static constexpr std::size_t extent = N;
};

struct integral_result {};
struct range_result {};

namespace detail {

template<class T>
concept member_describable = requires(T&& object) {
    std::forward<T>(object).describe();
};

template<class T>
concept non_bool_integral =
    std::integral<std::remove_cvref_t<T>> && !std::same_as<std::remove_cvref_t<T>, bool>;

} // namespace detail

struct describe_fn {
    template<class T>
        requires detail::member_describable<T>
    constexpr member_result operator()(T&& object) const
        noexcept(noexcept(std::forward<T>(object).describe()))
    {
        return {std::forward<T>(object).describe()};
    }

    template<std::size_t N>
    constexpr literal_result<N> operator()(const char (&)[N]) const noexcept {
        return {};
    }

    template<class T>
        requires (!detail::member_describable<T> && detail::non_bool_integral<T>)
    constexpr integral_result operator()(T&&) const noexcept {
        return {};
    }

    template<class T>
        requires (!detail::member_describable<T> && !detail::non_bool_integral<T>
            && std::ranges::range<T>)
    constexpr range_result operator()(T&&) const noexcept {
        return {};
    }
};

inline constexpr describe_fn describe{};

} // namespace c04_overload
