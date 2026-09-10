#pragma once

#include "03_interface.hpp"
#include <compare>
#include <concepts>
#include <memory>

namespace my::views {

template<std::integral W>
class iota_view : public my::ranges::view_interface<iota_view<W>> {
    W first_{};
    W last_{};
public:
    iota_view() = default;
    iota_view(W first, W last) : first_(first), last_(last) {}

    struct iterator {
        using value_type = W;
        using difference_type = std::ptrdiff_t;
        using iterator_concept = std::random_access_iterator_tag;
        W value{};
        W operator*() const { return value; }
        iterator& operator++() { ++value; return *this; }
        iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
        iterator& operator--() { --value; return *this; }
        iterator operator--(int) { auto tmp = *this; --*this; return tmp; }
        iterator& operator+=(difference_type n) { value += static_cast<W>(n); return *this; }
        iterator& operator-=(difference_type n) { value -= static_cast<W>(n); return *this; }
        W operator[](difference_type n) const { return value + static_cast<W>(n); }
        friend iterator operator+(iterator it, difference_type n) { it += n; return it; }
        friend iterator operator+(difference_type n, iterator it) { it += n; return it; }
        friend iterator operator-(iterator it, difference_type n) { it -= n; return it; }
        friend difference_type operator-(iterator a, iterator b) { return static_cast<difference_type>(a.value - b.value); }
        auto operator<=>(const iterator&) const = default;
        bool operator==(const iterator&) const = default;
    };

    iterator begin() const { return {first_}; }
    iterator end() const { return {last_}; }
};

inline constexpr auto iota = [](auto first, auto last) { return iota_view{first, last}; };

template<class T>
class single_view : public my::ranges::view_interface<single_view<T>> {
    T value_;
public:
    explicit single_view(T value) : value_(std::move(value)) {}
    T* begin() noexcept { return std::addressof(value_); }
    T* end() noexcept { return begin() + 1; }
    const T* begin() const noexcept { return std::addressof(value_); }
    const T* end() const noexcept { return begin() + 1; }
    static constexpr std::size_t size() noexcept { return 1; }
};

inline constexpr auto single = [](auto value) { return single_view{std::move(value)}; };

} // namespace my::views

namespace my::ranges {
template<std::integral W>
inline constexpr bool enable_view<my::views::iota_view<W>> = true;
template<std::integral W>
inline constexpr bool enable_borrowed_range<my::views::iota_view<W>> = true;
template<class T>
inline constexpr bool enable_view<my::views::single_view<T>> = true;
} // namespace my::ranges
