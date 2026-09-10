#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>

namespace c06_e1 {
namespace detail {

template<class R>
concept safe_range_object =
    std::is_lvalue_reference_v<R&&> ||
    std::ranges::enable_borrowed_range<std::remove_cvref_t<R>>;

template<class R>
concept array_range = std::is_bounded_array_v<std::remove_reference_t<R>>;

template<class R>
concept has_member_begin = requires(R& range) {
    { range.begin() } -> std::input_or_output_iterator;
};

template<class R>
concept has_adl_begin = (!has_member_begin<R>) && requires(R& range) {
    { begin(range) } -> std::input_or_output_iterator;
};

} // namespace detail

struct my_begin_fn {
    template<class R>
        requires detail::safe_range_object<R> && detail::array_range<R>
    constexpr auto operator()(R&& range) const noexcept {
        return range;
    }

    template<class R>
        requires detail::safe_range_object<R> && detail::has_member_begin<std::remove_reference_t<R>>
    constexpr auto operator()(R&& range) const noexcept(noexcept(range.begin())) {
        return range.begin();
    }

    template<class R>
        requires detail::safe_range_object<R> && detail::has_adl_begin<std::remove_reference_t<R>>
    constexpr auto operator()(R&& range) const noexcept(noexcept(begin(range))) {
        return begin(range);
    }
};

inline constexpr my_begin_fn my_begin{};

} // namespace c06_e1
