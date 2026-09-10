#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>

namespace c06_e1 {
namespace detail {

template<class R>
concept borrowed_or_lvalue =
    std::is_lvalue_reference_v<R&&> ||
    std::ranges::enable_borrowed_range<std::remove_cvref_t<R>>;

template<class R>
concept bounded_array = std::is_bounded_array_v<std::remove_reference_t<R>>;

template<class R>
concept member_begin = requires(R& range) {
    { range.begin() } -> std::input_or_output_iterator;
};

template<class R>
concept free_begin = (!member_begin<R>) && requires(R& range) {
    { begin(range) } -> std::input_or_output_iterator;
};

} // namespace detail

class my_begin_fn {
public:
    template<class R>
        requires detail::borrowed_or_lvalue<R> && detail::bounded_array<R>
    constexpr auto operator()(R&& range) const noexcept {
        return range;
    }

    template<class R>
        requires detail::borrowed_or_lvalue<R> && detail::member_begin<std::remove_reference_t<R>>
    constexpr auto operator()(R&& range) const noexcept(noexcept(range.begin())) {
        return range.begin();
    }

    template<class R>
        requires detail::borrowed_or_lvalue<R> && detail::free_begin<std::remove_reference_t<R>>
    constexpr auto operator()(R&& range) const noexcept(noexcept(begin(range))) {
        return begin(range);
    }
};

inline constexpr my_begin_fn my_begin{};

} // namespace c06_e1
