#pragma once

#include "01_cpo.hpp"
#include <concepts>
#include <iterator>
#include <type_traits>

namespace my::ranges {

template<class R>
concept range = requires(R& r) { my::ranges::begin(r); my::ranges::end(r); };
template<class R>
concept view = range<R> && std::movable<R> && enable_view<std::remove_cvref_t<R>>;
template<class R>
concept input_range = range<R> && std::input_iterator<iterator_t<R>>;
template<class R>
concept forward_range = input_range<R> && std::forward_iterator<iterator_t<R>>;
template<class R>
concept borrowed_range = range<R> &&
    (std::is_lvalue_reference_v<R> || enable_borrowed_range<std::remove_cvref_t<R>>);

} // namespace my::ranges
