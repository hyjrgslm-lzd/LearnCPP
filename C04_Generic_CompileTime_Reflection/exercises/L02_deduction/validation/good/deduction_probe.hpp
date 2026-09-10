#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace l02 {

template<class T>
struct type_token {
    using type = T;
};

template<class T, std::size_t N>
struct array_token {
    using element_type = T;
    static constexpr std::size_t extent = N;
};

template<class T>
constexpr auto by_value_token(T) -> type_token<T> {
    return {};
}

template<class T>
constexpr auto by_ref_token(T&) -> type_token<T> {
    return {};
}

template<class T, std::size_t N>
constexpr auto array_ref_token(T (&)[N]) -> array_token<T, N> {
    return {};
}

template<class T>
constexpr decltype(auto) identity_decltype_auto(T&& value) {
    return std::forward<T>(value);
}

template<class T>
constexpr type_token<T> freeze_as(std::type_identity_t<T>) {
    return {};
}

} // namespace l02
