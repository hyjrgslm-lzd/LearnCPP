#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace c06_g1 {

template<std::ranges::view V>
class my_take_view : public std::ranges::view_interface<my_take_view<V>> {
    using diff_t = std::ranges::range_difference_t<V>;

    struct guarded_iterator {
        using value_type = std::ranges::range_value_t<V>;
        using difference_type = std::ranges::range_difference_t<V>;
        using iterator_concept = std::input_iterator_tag;
        std::ranges::iterator_t<V> it{};
        std::ranges::sentinel_t<V> stop{};
        diff_t left{};
        decltype(auto) operator*() const { return *it; }
        guarded_iterator& operator++() {
            if (left != 0 && it != stop) {
                ++it;
                --left;
            }
            return *this;
        }
        void operator++(int) { ++*this; }
        friend bool operator==(const guarded_iterator& it, std::default_sentinel_t) {
            return it.left == 0 || it.it == it.stop;
        }
    };

public:
    my_take_view() = default;
    my_take_view(V base, diff_t count) : base_(std::move(base)), count_(count) {
        if (count < 0) throw std::invalid_argument("negative my_take_view count");
    }

    auto begin() {
        if constexpr (std::ranges::random_access_range<V> && std::ranges::sized_range<V>) {
            return std::ranges::begin(base_);
        } else if constexpr (std::ranges::sized_range<V>) {
            return std::counted_iterator(std::ranges::begin(base_), limit());
        } else {
            return guarded_iterator{std::ranges::begin(base_), std::ranges::end(base_), count_};
        }
    }

    auto end() {
        if constexpr (std::ranges::random_access_range<V> && std::ranges::sized_range<V>) {
            return std::ranges::begin(base_) + limit();
        } else {
            return std::default_sentinel;
        }
    }

    auto size() requires std::ranges::sized_range<V> {
        return static_cast<std::ranges::range_size_t<V>>(limit());
    }

private:
    diff_t limit() {
        return std::min(count_, static_cast<diff_t>(std::ranges::size(base_)));
    }

    V base_{};
    diff_t count_{};
};

template<std::ranges::viewable_range R>
my_take_view(R&&, std::ranges::range_difference_t<R>) -> my_take_view<std::views::all_t<R>>;

} // namespace c06_g1
