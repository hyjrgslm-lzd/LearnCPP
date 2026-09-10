#pragma once

#include <functional>
#include <optional>
#include <ranges>
#include <utility>

namespace h1 {

template<class T>
class non_propagating_cache {
    std::optional<T> slot_;

public:
    non_propagating_cache() = default;
    non_propagating_cache(const non_propagating_cache&) = default;
    non_propagating_cache(non_propagating_cache&&) = default;
    non_propagating_cache& operator=(const non_propagating_cache&) = default;
    non_propagating_cache& operator=(non_propagating_cache&&) = default;
    bool has_value() const noexcept { return slot_.has_value(); }
    T& operator*() noexcept { return *slot_; }
    template<class... Args>
    T& emplace(Args&&... args) { slot_.emplace(std::forward<Args>(args)...); return *slot_; }
};

template<std::ranges::forward_range V, class Pred>
    requires std::ranges::view<V> &&
             std::indirect_unary_predicate<Pred&, std::ranges::iterator_t<V>>
class my_filter_view : public std::ranges::view_interface<my_filter_view<V, Pred>> {
    V base_{};
    Pred pred_{};
    non_propagating_cache<std::ranges::iterator_t<V>> first_;

public:
    my_filter_view() = default;
    my_filter_view(V base, Pred pred) : base_(std::move(base)), pred_(std::move(pred)) {}

    auto begin() {
        if (!first_.has_value()) {
            auto it = std::ranges::begin(base_);
            auto last = std::ranges::end(base_);
            while (it != last && !std::invoke(pred_, *it)) {
                ++it;
            }
            first_.emplace(it);
        }
        return *first_;
    }

    auto end() { return std::ranges::end(base_); }
};

template <std::ranges::viewable_range R, class Pred>
my_filter_view(R&&, Pred) -> my_filter_view<std::views::all_t<R>, Pred>;

void run_filter_cache_checks();

} // namespace h1
