#pragma once

#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace c06_g2 {

namespace detail {
template<class It>
concept has_iterator_category = requires { typename std::iterator_traits<It>::iterator_category; };

template<class It, class Ref>
struct iterator_category_for {
    using iterator_category = std::input_iterator_tag;
};

template<class It, class Ref>
    requires std::is_lvalue_reference_v<Ref> && has_iterator_category<It>
struct iterator_category_for<It, Ref> {
    using iterator_category = typename std::iterator_traits<It>::iterator_category;
};
} // namespace detail

template<std::ranges::view V, std::move_constructible F>
    requires std::is_object_v<F> && std::regular_invocable<F&, std::ranges::range_reference_t<V>>
class my_transform_view : public std::ranges::view_interface<my_transform_view<V, F>> {
    class sentinel;

    class iterator : public detail::iterator_category_for<
        std::ranges::iterator_t<V>,
        std::invoke_result_t<F&, std::ranges::range_reference_t<V>>> {
        using base_iterator = std::ranges::iterator_t<V>;
        F* function_{};
        base_iterator current_{};
    public:
        using reference = std::invoke_result_t<F&, std::ranges::range_reference_t<V>>;
        using value_type = std::remove_cvref_t<reference>;
        using difference_type = std::ranges::range_difference_t<V>;
        using iterator_concept =
            std::conditional_t<std::random_access_iterator<base_iterator>, std::random_access_iterator_tag,
            std::conditional_t<std::bidirectional_iterator<base_iterator>, std::bidirectional_iterator_tag,
            std::conditional_t<std::forward_iterator<base_iterator>, std::forward_iterator_tag,
            std::input_iterator_tag>>>;

        iterator() = default;
        iterator(F& function, base_iterator current) : function_(&function), current_(std::move(current)) {}
        reference operator*() const { return static_cast<value_type>(*current_); }
        iterator& operator++() { ++current_; return *this; }
        iterator operator++(int) requires std::forward_iterator<base_iterator> { auto old = *this; ++*this; return old; }
        void operator++(int) { ++*this; }
        iterator& operator--() requires std::bidirectional_iterator<base_iterator> { --current_; return *this; }
        iterator operator--(int) requires std::bidirectional_iterator<base_iterator> { auto old = *this; --*this; return old; }
        iterator& operator+=(difference_type n) requires std::random_access_iterator<base_iterator> { current_ += n; return *this; }
        iterator& operator-=(difference_type n) requires std::random_access_iterator<base_iterator> { current_ -= n; return *this; }
        iterator operator+(difference_type n) const requires std::random_access_iterator<base_iterator> { auto out = *this; out += n; return out; }
        friend iterator operator+(difference_type n, iterator it) requires std::random_access_iterator<base_iterator> { return it + n; }
        iterator operator-(difference_type n) const requires std::random_access_iterator<base_iterator> { auto out = *this; out -= n; return out; }
        difference_type operator-(const iterator& other) const requires std::random_access_iterator<base_iterator> { return current_ - other.current_; }
        reference operator[](difference_type n) const requires std::random_access_iterator<base_iterator> { return std::invoke(*function_, current_[n]); }
        bool operator==(const iterator& other) const requires std::equality_comparable<base_iterator> { return current_ == other.current_; }
        auto operator<=>(const iterator& other) const requires std::random_access_iterator<base_iterator> { return current_ <=> other.current_; }
        friend class sentinel;
    };

    class sentinel {
        std::ranges::sentinel_t<V> last_{};
        bool equal(const iterator& it) const { return it.current_ == last_; }
    public:
        sentinel() = default;
        explicit sentinel(std::ranges::sentinel_t<V> last) : last_(last) {}
        friend bool operator==(const iterator& it, const sentinel& s) { return s.equal(it); }
        friend bool operator==(const sentinel& s, const iterator& it) { return s.equal(it); }
    };

public:
    my_transform_view() = default;
    my_transform_view(V base, F function) : base_(std::move(base)), function_(std::move(function)) {}
    iterator begin() { return iterator{function_, std::ranges::begin(base_)}; }
    auto end() {
        if constexpr (std::ranges::common_range<V>) return iterator{function_, std::ranges::end(base_)};
        else return sentinel{std::ranges::end(base_)};
    }
    auto size() requires std::ranges::sized_range<V> { return std::ranges::size(base_); }
private:
    V base_{};
    F function_{};
};

template<std::ranges::viewable_range R, class F>
my_transform_view(R&&, F) -> my_transform_view<std::views::all_t<R>, F>;

template<std::move_constructible F>
class my_transform_closure : public std::ranges::range_adaptor_closure<my_transform_closure<F>> {
public:
    explicit my_transform_closure(F function) : function_(std::move(function)) {}
    template<std::ranges::viewable_range R>
        requires std::copy_constructible<F> && std::regular_invocable<F&, std::ranges::range_reference_t<R>>
    auto operator()(R&& range) const& {
        return my_transform_view{std::views::all(std::forward<R>(range)), function_};
    }
    template<std::ranges::viewable_range R>
        requires std::regular_invocable<F&, std::ranges::range_reference_t<R>>
    auto operator()(R&& range) && {
        return my_transform_view{std::views::all(std::forward<R>(range)), std::move(function_)};
    }
private:
    F function_;
};

struct my_transform_fn {
    template<std::move_constructible F>
    auto operator()(F function) const {
        return my_transform_closure<F>{std::move(function)};
    }
    template<std::ranges::viewable_range R, std::move_constructible F>
        requires std::regular_invocable<F&, std::ranges::range_reference_t<R>>
    auto operator()(R&& range, F function) const {
        return my_transform_view{std::views::all(std::forward<R>(range)), std::move(function)};
    }
};

inline constexpr my_transform_fn my_transform{};

} // namespace c06_g2