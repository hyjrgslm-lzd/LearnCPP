#pragma once

#include <ranges>
#include <utility>

namespace c06_g2 {

template<class V, class F>
using my_transform_view = std::ranges::transform_view<V, F>;

struct my_transform_fn {
    template<class F>
    auto operator()(F function) const {
        return std::views::transform(std::move(function));
    }

    template<std::ranges::viewable_range R, class F>
    auto operator()(R&& range, F function) const {
        return std::views::transform(std::forward<R>(range), std::move(function));
    }
};

inline constexpr my_transform_fn my_transform{};

} // namespace c06_g2