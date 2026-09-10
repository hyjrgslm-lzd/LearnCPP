#pragma once

#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace c06_g3 {

template<std::ranges::view V>
class my_enumerate_view : public std::ranges::view_interface<my_enumerate_view<V>> {
public:
    using index_type = std::ranges::range_difference_t<V>;
private:
    class sentinel;

    class iterator {
        std::ranges::iterator_t<V> current_{};
        index_type index_{};
    public:
        using value_type = std::pair<index_type, std::ranges::range_value_t<V>>;
        using reference = std::pair<index_type, std::ranges::range_reference_t<V>>;
        using difference_type = std::ranges::range_difference_t<V>;
        using iterator_concept =
            std::conditional_t<std::random_access_iterator<std::ranges::iterator_t<V>>, std::random_access_iterator_tag,
            std::conditional_t<std::bidirectional_iterator<std::ranges::iterator_t<V>>, std::bidirectional_iterator_tag,
            std::conditional_t<std::forward_iterator<std::ranges::iterator_t<V>>, std::forward_iterator_tag,
            std::input_iterator_tag>>>;
        using iterator_category = std::input_iterator_tag;

        iterator() = default;
        iterator(std::ranges::iterator_t<V> current, index_type index) : current_(std::move(current)), index_(index) {}
        reference operator*() const { return {index_, *current_}; }
        iterator& operator++() { ++current_; ++index_; return *this; }
        iterator operator++(int) requires std::forward_iterator<std::ranges::iterator_t<V>> { auto old = *this; ++*this; return old; }
        void operator++(int) { ++*this; }
        iterator& operator--() requires std::bidirectional_iterator<std::ranges::iterator_t<V>> { --current_; --index_; return *this; }
        iterator operator--(int) requires std::bidirectional_iterator<std::ranges::iterator_t<V>> { auto old = *this; --*this; return old; }
        iterator& operator+=(difference_type n) requires std::random_access_iterator<std::ranges::iterator_t<V>> { current_ += n; index_ += n; return *this; }
        iterator& operator-=(difference_type n) requires std::random_access_iterator<std::ranges::iterator_t<V>> { current_ -= n; index_ -= n; return *this; }
        iterator operator+(difference_type n) const requires std::random_access_iterator<std::ranges::iterator_t<V>> { auto out = *this; out += n; return out; }
        friend iterator operator+(difference_type n, iterator it) requires std::random_access_iterator<std::ranges::iterator_t<V>> { return it + n; }
        iterator operator-(difference_type n) const requires std::random_access_iterator<std::ranges::iterator_t<V>> { auto out = *this; out -= n; return out; }
        difference_type operator-(const iterator& other) const requires std::random_access_iterator<std::ranges::iterator_t<V>> { return current_ - other.current_; }
        reference operator[](difference_type n) const requires std::random_access_iterator<std::ranges::iterator_t<V>> { return {index_ + n, current_[n]}; }
        bool operator==(const iterator& other) const requires std::equality_comparable<std::ranges::iterator_t<V>> { return current_ == other.current_; }
        auto operator<=>(const iterator& other) const requires std::random_access_iterator<std::ranges::iterator_t<V>> { return current_ <=> other.current_; }
        friend class sentinel;
        friend auto iter_move(const iterator& it) noexcept(noexcept(std::ranges::iter_move(it.current_))) {
            return std::pair<index_type, std::ranges::range_rvalue_reference_t<V>>{it.index_, std::ranges::iter_move(it.current_)};
        }
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
    my_enumerate_view() = default;
    explicit my_enumerate_view(V base) : base_(std::move(base)) {}
    iterator begin() { return iterator{std::ranges::begin(base_), 0}; }
    auto end() {
        if constexpr (std::ranges::common_range<V> && std::ranges::sized_range<V>) {
            return iterator{std::ranges::end(base_), static_cast<index_type>(std::ranges::size(base_) + 0)};
        } else {
            return sentinel{std::ranges::end(base_)};
        }
    }
    auto size() requires std::ranges::sized_range<V> { return std::ranges::size(base_); }
private:
    V base_{};
};

template<std::ranges::viewable_range R>
my_enumerate_view(R&&) -> my_enumerate_view<std::views::all_t<R>>;

struct my_enumerate_closure : std::ranges::range_adaptor_closure<my_enumerate_closure> {
    template<std::ranges::viewable_range R>
    auto operator()(R&& range) const {
        return my_enumerate_view{std::views::all(std::forward<R>(range))};
    }
};

struct my_enumerate_fn {
    my_enumerate_closure operator()() const { return {}; }
    template<std::ranges::viewable_range R>
    auto operator()(R&& range) const {
        return my_enumerate_view{std::views::all(std::forward<R>(range))};
    }
};

inline constexpr my_enumerate_fn my_enumerate{};

} // namespace c06_g3

// Intentionally missing enable_borrowed_range forwarding for validation/bad.