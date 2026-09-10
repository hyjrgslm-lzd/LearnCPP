#pragma once

#include <functional>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

namespace h1 {

template<class T>
class non_propagating_cache {
    std::optional<T> value_;

public:
    non_propagating_cache() = default;
    non_propagating_cache(const non_propagating_cache&) noexcept {}
    non_propagating_cache(non_propagating_cache&&) noexcept {}
    non_propagating_cache& operator=(const non_propagating_cache&) noexcept { value_.reset(); return *this; }
    non_propagating_cache& operator=(non_propagating_cache&&) noexcept { value_.reset(); return *this; }
    bool has_value() const noexcept { return value_.has_value(); }
    T& operator*() noexcept { return *value_; }
    template<class... Args>
    T& emplace(Args&&... args) { value_.emplace(std::forward<Args>(args)...); return *value_; }
};

template<std::ranges::forward_range V, class Pred>
    requires std::ranges::view<V> &&
             std::indirect_unary_predicate<Pred&, std::ranges::iterator_t<V>>
class my_filter_view : public std::ranges::view_interface<my_filter_view<V, Pred>> {
    V base_{};
    Pred pred_{};
    non_propagating_cache<std::ranges::iterator_t<V>> first_;

    auto satisfy(std::ranges::iterator_t<V> it) {
        auto last = std::ranges::end(base_);
        while (it != last && !std::invoke(pred_, *it)) {
            ++it;
        }
        return it;
    }

public:
    my_filter_view() = default;
    my_filter_view(V base, Pred pred) : base_(std::move(base)), pred_(std::move(pred)) {}

    class iterator {
        my_filter_view* owner_{};
        std::ranges::iterator_t<V> it_{};

    public:
        using value_type = std::ranges::range_value_t<V>;
        using difference_type = std::ranges::range_difference_t<V>;
        using iterator_concept = std::conditional_t<std::ranges::bidirectional_range<V>,
            std::bidirectional_iterator_tag, std::forward_iterator_tag>;

        iterator() = default;
        iterator(my_filter_view* owner, std::ranges::iterator_t<V> it) : owner_(owner), it_(std::move(it)) {}
        decltype(auto) operator*() const { return *it_; }
        iterator& operator++() { it_ = owner_->satisfy(++it_); return *this; }
        iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
        iterator& operator--() requires std::ranges::bidirectional_range<V> {
            do { --it_; } while (!std::invoke(owner_->pred_, *it_));
            return *this;
        }
        friend bool operator==(const iterator& a, const iterator& b) { return a.it_ == b.it_; }
        friend bool operator==(const iterator& a, const std::ranges::sentinel_t<V>& b) { return a.it_ == b; }
    };

    iterator begin() {
        if (!first_.has_value()) {
            first_.emplace(satisfy(std::ranges::begin(base_)));
        }
        return iterator{this, *first_};
    }

    auto end() { return std::ranges::end(base_); }
};

template <std::ranges::viewable_range R, class Pred>
my_filter_view(R&&, Pred) -> my_filter_view<std::views::all_t<R>, Pred>;

void run_filter_cache_checks();

} // namespace h1
