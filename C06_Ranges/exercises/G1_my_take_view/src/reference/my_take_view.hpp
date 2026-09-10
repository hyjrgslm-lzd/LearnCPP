#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace c06_g1 {

template<std::ranges::view V>
class my_take_view : public std::ranges::view_interface<my_take_view<V>> {
    using difference_type = std::ranges::range_difference_t<V>;

    class input_iterator {
        std::ranges::iterator_t<V> current_{};
        std::ranges::sentinel_t<V> last_{};
        difference_type remaining_{};
    public:
        using value_type = std::ranges::range_value_t<V>;
        using difference_type = std::ranges::range_difference_t<V>;
        using iterator_concept = std::input_iterator_tag;
        input_iterator() = default;
        input_iterator(std::ranges::iterator_t<V> current, std::ranges::sentinel_t<V> last,
                       difference_type remaining)
            : current_(std::move(current)), last_(std::move(last)), remaining_(remaining) {}
        decltype(auto) operator*() const { return *current_; }
        input_iterator& operator++() {
            if (remaining_ > 0 && current_ != last_) {
                ++current_;
                --remaining_;
            }
            return *this;
        }
        void operator++(int) { ++*this; }
        friend bool operator==(const input_iterator& it, std::default_sentinel_t) {
            return it.remaining_ <= 0 || it.current_ == it.last_;
        }
    };

public:
    my_take_view() = default;
    constexpr my_take_view(V base, difference_type count) : base_(std::move(base)), count_(count) {
        if (count < 0) throw std::invalid_argument("my_take_view requires a non-negative count");
    }

    constexpr auto begin() {
        if constexpr (std::ranges::random_access_range<V> && std::ranges::sized_range<V>) {
            return std::ranges::begin(base_);
        } else if constexpr (std::ranges::sized_range<V>) {
            return std::counted_iterator{std::ranges::begin(base_), clamped_count()};
        } else {
            return input_iterator{std::ranges::begin(base_), std::ranges::end(base_), count_};
        }
    }

    constexpr auto end() {
        if constexpr (std::ranges::random_access_range<V> && std::ranges::sized_range<V>) {
            return std::ranges::begin(base_) + clamped_count();
        } else {
            return std::default_sentinel;
        }
    }

    constexpr auto size() requires std::ranges::sized_range<V> {
        return static_cast<std::ranges::range_size_t<V>>(clamped_count());
    }

private:
    constexpr difference_type clamped_count() {
        auto size = static_cast<difference_type>(std::ranges::size(base_));
        return std::min(count_, size);
    }

    V base_{};
    difference_type count_{};
};

template<std::ranges::viewable_range R>
my_take_view(R&&, std::ranges::range_difference_t<R>) -> my_take_view<std::views::all_t<R>>;

} // namespace c06_g1
