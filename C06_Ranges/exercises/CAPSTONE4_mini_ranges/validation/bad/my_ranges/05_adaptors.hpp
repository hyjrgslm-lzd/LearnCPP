#pragma once

#include "04_factories.hpp"
#include <functional>
#include <type_traits>

namespace my::views {

template<my::ranges::view V, class F>
class transform_view : public my::ranges::view_interface<transform_view<V, F>> {
    V base_;
    F fun_;
public:
    transform_view(V base, F fun) : base_(std::move(base)), fun_(std::move(fun)) {}

    struct iterator {
        using base_iterator = my::ranges::iterator_t<V>;
        using value_type = std::remove_cvref_t<std::invoke_result_t<F&, my::ranges::range_reference_t<V>>>;
        using difference_type = std::iter_difference_t<base_iterator>;
        using iterator_concept = std::conditional_t<std::random_access_iterator<base_iterator>,
            std::random_access_iterator_tag, std::input_iterator_tag>;
        using iterator_category = std::input_iterator_tag;
        base_iterator current{};
        F* fun{};
        decltype(auto) operator*() const { return std::invoke(*fun, *current); }
        iterator& operator++() { ++current; return *this; }
        iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
        iterator& operator--() requires std::bidirectional_iterator<base_iterator> { --current; return *this; }
        iterator& operator+=(difference_type n) requires std::random_access_iterator<base_iterator> { current += n; return *this; }
        iterator& operator-=(difference_type n) requires std::random_access_iterator<base_iterator> { current -= n; return *this; }
        decltype(auto) operator[](difference_type n) const requires std::random_access_iterator<base_iterator> { return std::invoke(*fun, current[n]); }
        friend iterator operator+(iterator it, difference_type n) requires std::random_access_iterator<base_iterator> { it += n; return it; }
        friend iterator operator+(difference_type n, iterator it) requires std::random_access_iterator<base_iterator> { it += n; return it; }
        friend iterator operator-(iterator it, difference_type n) requires std::random_access_iterator<base_iterator> { it -= n; return it; }
        friend difference_type operator-(const iterator& a, const iterator& b) requires std::random_access_iterator<base_iterator> { return a.current - b.current; }
        friend auto operator<=>(const iterator& a, const iterator& b) requires std::random_access_iterator<base_iterator> { return a.current <=> b.current; }
        friend bool operator==(const iterator& a, const iterator& b) { return a.current == b.current; }
    };

    iterator begin() { return {my::ranges::begin(base_), std::addressof(fun_)}; }
    iterator end() { return {my::ranges::end(base_), std::addressof(fun_)}; }
};

template<my::ranges::view V>
class take_view : public my::ranges::view_interface<take_view<V>> {
    V base_;
    std::ptrdiff_t count_{};
public:
    take_view(V base, std::ptrdiff_t count) : base_(std::move(base)), count_(count) {}

    struct sentinel {
        my::ranges::sentinel_t<V> last;
    };
    struct iterator {
        my::ranges::iterator_t<V> current{};
        std::ptrdiff_t remaining{};
        using value_type = std::iter_value_t<my::ranges::iterator_t<V>>;
        using difference_type = std::iter_difference_t<my::ranges::iterator_t<V>>;
        using iterator_concept = std::input_iterator_tag;
        decltype(auto) operator*() const { return *current; }
        iterator& operator++() { ++current; --remaining; return *this; }
        void operator++(int) { ++*this; }
        friend bool operator==(const iterator& it, const sentinel& s) { return it.remaining < 0 || it.current == s.last; }
    };

    iterator begin() { return {my::ranges::begin(base_), count_}; }
    sentinel end() { return {my::ranges::end(base_)}; }
};

template<class F>
struct transform_closure : my::ranges::range_adaptor_closure<transform_closure<F>> {
    F fun;
    explicit transform_closure(F f) : fun(std::move(f)) {}
    template<my::ranges::view V>
    auto operator()(V&& v) const { return transform_view<std::remove_cvref_t<V>, F>{std::forward<V>(v), fun}; }
};

struct take_closure : my::ranges::range_adaptor_closure<take_closure> {
    std::ptrdiff_t count;
    explicit take_closure(std::ptrdiff_t n) : count(n) {}
    template<my::ranges::view V>
    auto operator()(V&& v) const { return take_view<std::remove_cvref_t<V>>{std::forward<V>(v), count}; }
};

inline constexpr auto transform = [](auto fun) { return transform_closure{std::move(fun)}; };
inline constexpr auto take = [](auto count) { return take_closure{static_cast<std::ptrdiff_t>(count)}; };

} // namespace my::views

namespace my::ranges {
template<class V, class F>
inline constexpr bool enable_view<my::views::transform_view<V, F>> = true;
template<class V>
inline constexpr bool enable_view<my::views::take_view<V>> = true;
} // namespace my::ranges
