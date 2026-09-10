#pragma once

#include <utility>

namespace c04_lookup::detail {

void inspect();

template<class T>
concept member_inspectable = requires(T&& object) {
    std::forward<T>(object).inspect();
};

template<class T>
concept adl_inspectable = requires(T&& object) {
    inspect(std::forward<T>(object));
};

struct inspect_fn {
    template<class T>
        requires member_inspectable<T>
    constexpr decltype(auto) operator()(T&& object) const
        noexcept(noexcept(std::forward<T>(object).inspect()))
    {
        return std::forward<T>(object).inspect();
    }

    template<class T>
        requires (!member_inspectable<T> && adl_inspectable<T>)
    constexpr decltype(auto) operator()(T&& object) const
        noexcept(noexcept(inspect(std::forward<T>(object))))
    {
        return inspect(std::forward<T>(object));
    }

    template<class T>
    constexpr int operator()(T&&) const noexcept {
        return 0;
    }
};

} // namespace c04_lookup::detail

namespace c04_lookup {

inline constexpr detail::inspect_fn inspect{};

} // namespace c04_lookup
