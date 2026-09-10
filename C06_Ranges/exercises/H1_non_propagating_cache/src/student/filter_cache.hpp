#pragma once

#include <ranges>
#include <utility>

namespace h1 {

template<class T>
class non_propagating_cache {
public:
    bool has_value() const noexcept { return false; }
    void reset() noexcept {}
};

template<class V, class Pred>
class my_filter_view : public std::ranges::view_interface<my_filter_view<V, Pred>> {
    V base_;
    Pred pred_;

public:
    my_filter_view() = default;
    my_filter_view(V base, Pred pred) : base_(std::move(base)), pred_(std::move(pred)) {}

    auto begin() { return std::ranges::begin(base_); }
    auto end() { return std::ranges::end(base_); }
};

template <std::ranges::viewable_range R, class Pred>
my_filter_view(R&&, Pred) -> my_filter_view<std::views::all_t<R>, Pred>;

void run_filter_cache_checks();

} // namespace h1
