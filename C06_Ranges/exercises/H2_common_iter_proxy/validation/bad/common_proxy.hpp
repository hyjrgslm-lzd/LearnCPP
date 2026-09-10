#pragma once

#include <iterator>
#include <tuple>
#include <variant>
#include <utility>

namespace h2 {

template<std::input_or_output_iterator I, std::sentinel_for<I> S>
    requires (!std::same_as<I, S> && std::copyable<I>)
class my_common_iterator {
    std::variant<I, S> state_;

public:
    explicit my_common_iterator(I it) : state_(std::in_place_type<I>, std::move(it)) {}
    explicit my_common_iterator(S end) : state_(std::in_place_type<S>, std::move(end)) {}
    decltype(auto) operator*() const { return *std::get<I>(state_); }
    my_common_iterator& operator++() { ++std::get<I>(state_); return *this; }
    void operator++(int) { ++*this; }
    friend bool operator==(const my_common_iterator& a, const my_common_iterator& b) {
        if (a.state_.index() == 0 && b.state_.index() == 1) return std::get<I>(a.state_) == std::get<S>(b.state_);
        if (a.state_.index() == 1 && b.state_.index() == 0) return std::get<I>(b.state_) == std::get<S>(a.state_);
        return a.state_.index() == b.state_.index();
    }
    using value_type = std::iter_value_t<I>;
    using difference_type = std::iter_difference_t<I>;
    using iterator_category = std::input_iterator_tag;
};

template<class It1, class It2>
struct ZipIterator {
    It1 it1_;
    It2 it2_;
    using value_type = std::tuple<std::iter_value_t<It1>, std::iter_value_t<It2>>;
    using difference_type = std::common_type_t<std::iter_difference_t<It1>, std::iter_difference_t<It2>>;
    using reference = std::tuple<std::iter_reference_t<It1>, std::iter_reference_t<It2>>;
    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    reference operator*() const { return reference(*it1_, *it2_); }
    ZipIterator& operator++() { ++it1_; ++it2_; return *this; }
    friend auto iter_move(const ZipIterator& it) {
        return std::tuple<std::iter_rvalue_reference_t<It1>, std::iter_rvalue_reference_t<It2>>(
            std::ranges::iter_move(it.it1_), std::ranges::iter_move(it.it2_));
    }
    friend void iter_swap(const ZipIterator&, const ZipIterator&) {}
};

void run_common_proxy_checks();

} // namespace h2
