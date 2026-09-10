#pragma once

#include "04_factories.hpp"

#include <functional>
#include <ranges>
#include <utility>

namespace my::views {

template<class V, class F>
using transform_view = std::ranges::transform_view<V, F>;

template<std::ranges::view V>
class take_view : public my::ranges::view_interface<take_view<V>> {
    V base_;
    std::ptrdiff_t count_{};

public:
    take_view() = default;
    take_view(V base, std::ptrdiff_t count) : base_(std::move(base)), count_(count) {}

    class iterator {
    public:
        std::ranges::iterator_t<V> current_{};
        std::ptrdiff_t remaining_{};

        using value_type = std::iter_value_t<std::ranges::iterator_t<V>>;
        using difference_type = std::iter_difference_t<std::ranges::iterator_t<V>>;
        using iterator_concept = typename std::ranges::iterator_t<V>::iterator_concept;

        iterator() = default;
        iterator(std::ranges::iterator_t<V> current, std::ptrdiff_t remaining)
            : current_(std::move(current)), remaining_(remaining) {}

        decltype(auto) operator*() const { return *current_; }
        iterator& operator++() { ++current_; --remaining_; return *this; }
        void operator++(int) { ++*this; }

        friend class take_view;
    };

    struct sentinel {
        std::ranges::sentinel_t<V> last_{};
    };

    iterator begin() { return iterator{std::ranges::begin(base_), count_}; }
    sentinel end() { return sentinel{std::ranges::end(base_)}; }

    friend bool operator==(const iterator& it, const sentinel& s) {
        return it.remaining_ <= 0 || it.current_ == s.last_;
    }
};

template<class F>
struct transform_closure : my::ranges::range_adaptor_closure<transform_closure<F>> {
    F fun;
    explicit transform_closure(F f) : fun(std::move(f)) {}

    template<class R>
    auto operator()(R&& r) const {
        return std::views::all(std::forward<R>(r)) | std::views::transform(fun);
    }
};

struct take_closure : my::ranges::range_adaptor_closure<take_closure> {
    std::ptrdiff_t count{};
    explicit take_closure(std::ptrdiff_t n) : count(n) {}

    template<class R>
    auto operator()(R&& r) const {
        return take_view<std::views::all_t<R>>{std::views::all(std::forward<R>(r)), count};
    }
};

template<class F>
auto transform(F f) { return transform_closure<F>{std::move(f)}; }

inline auto take(std::ptrdiff_t count) { return take_closure{count}; }

} // namespace my::views

namespace my::ranges {

template<class V>
inline constexpr bool enable_view<my::views::take_view<V>> = true;

} // namespace my::ranges
