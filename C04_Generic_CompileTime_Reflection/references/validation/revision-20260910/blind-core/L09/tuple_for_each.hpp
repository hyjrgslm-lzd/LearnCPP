#pragma once

#include <cstddef>
#include <tuple>
#include <utility>

namespace c04 {

template<class Tuple, class F, std::size_t... Is>
constexpr void for_each_tuple_impl(Tuple&& tuple, F& f, std::index_sequence<Is...>) {
    (static_cast<void>(f(std::get<Is>(std::forward<Tuple>(tuple)))), ...);
}

template<class Tuple, class F>
constexpr void for_each_tuple(Tuple&& tuple, F&& f) {
    constexpr auto size = std::tuple_size_v<std::remove_reference_t<Tuple>>;
    for_each_tuple_impl(std::forward<Tuple>(tuple), f, std::make_index_sequence<size>{});
}

} // namespace c04
