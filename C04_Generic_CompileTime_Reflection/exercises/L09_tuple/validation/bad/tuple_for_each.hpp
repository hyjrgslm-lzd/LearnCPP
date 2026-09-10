#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace c04 {
template<class Tuple, class F>
constexpr void for_each_tuple(Tuple&& tuple, F&& f) {
    constexpr auto size = std::tuple_size_v<std::remove_reference_t<Tuple>>;
    if constexpr (size == 0) {
        return;
    } else if constexpr (size == 1) {
        f(std::get<0>(std::forward<Tuple>(tuple)));
    } else {
        f(std::get<1>(std::forward<Tuple>(tuple)));
        f(std::get<0>(std::forward<Tuple>(tuple)));
    }
}
} // namespace c04
