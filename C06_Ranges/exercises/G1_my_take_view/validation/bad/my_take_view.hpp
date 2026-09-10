#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

namespace c06_g1 {

template<std::ranges::view V>
class my_take_view : public std::ranges::view_interface<my_take_view<V>> {
    using diff_t = std::ranges::range_difference_t<V>;
public:
    my_take_view() = default;
    my_take_view(V base, diff_t count) : base_(std::move(base)), count_(count) {}
    auto begin() {
        if constexpr (std::ranges::random_access_range<V>) return std::ranges::begin(base_);
        else return std::counted_iterator(std::ranges::begin(base_), count_);
    }
    auto end() {
        if constexpr (std::ranges::random_access_range<V>) return std::ranges::begin(base_) + std::min(count_, static_cast<diff_t>(std::ranges::size(base_)));
        else return std::default_sentinel;
    }
    auto size() requires std::ranges::sized_range<V> {
        return static_cast<std::ranges::range_size_t<V>>(std::min(count_, static_cast<diff_t>(std::ranges::size(base_))));
    }
private:
    V base_{};
    diff_t count_{};
};

template<std::ranges::viewable_range R>
my_take_view(R&&, std::ranges::range_difference_t<R>) -> my_take_view<std::views::all_t<R>>;

} // namespace c06_g1
