#pragma once

#include "03_interface.hpp"

#include <ranges>

namespace my::views {

template<class I = int>
using iota_view = std::ranges::iota_view<I, I>;

template<class T>
using single_view = std::ranges::single_view<T>;

inline constexpr auto iota = []<class I>(I first, I last) {
    return std::ranges::iota_view<I, I>{first, last};
};

inline constexpr auto single = []<class T>(T value) {
    return std::ranges::single_view<T>{std::move(value)};
};

} // namespace my::views
