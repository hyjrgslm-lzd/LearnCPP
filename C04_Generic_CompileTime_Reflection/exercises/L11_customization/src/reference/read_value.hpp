#pragma once

#include <utility>

namespace c04::read_value_detail {

void read_value();

template<class T>
concept member_readable = requires(T&& object) {
    std::forward<T>(object).read_value();
};

template<class T>
concept adl_readable = requires(T&& object) {
    read_value(std::forward<T>(object));
};

struct read_value_fn {
    template<class T>
        requires member_readable<T>
    constexpr decltype(auto) operator()(T&& object) const
        noexcept(noexcept(std::forward<T>(object).read_value()))
    {
        return std::forward<T>(object).read_value();
    }

    template<class T>
        requires (!member_readable<T> && adl_readable<T>)
    constexpr decltype(auto) operator()(T&& object) const
        noexcept(noexcept(read_value(std::forward<T>(object))))
    {
        return read_value(std::forward<T>(object));
    }
};

} // namespace c04::read_value_detail

namespace c04 {

inline constexpr read_value_detail::read_value_fn read_value{};

} // namespace c04
