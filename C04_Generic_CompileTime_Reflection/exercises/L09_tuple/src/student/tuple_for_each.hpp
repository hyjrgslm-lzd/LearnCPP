#pragma once

namespace c04 {

template<class Tuple, class F>
constexpr void for_each_tuple(Tuple&&, F&&) {}

} // namespace c04
