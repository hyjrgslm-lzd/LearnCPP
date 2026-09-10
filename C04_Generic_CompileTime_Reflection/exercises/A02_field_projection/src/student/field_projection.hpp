#pragma once
#include <projection_schema.hpp>
#include <type_traits>
#include <tuple>
#include <utility>

namespace c04_projection {
template<fixed_string Name, class T>
constexpr decltype(auto) get(T&& object) {
    if constexpr (requires { object.id; } && Name.value[0] == 'i') return (std::forward<T>(object).id);
    else if constexpr (requires { object.active; } && Name.value[0] == 'a') return (std::forward<T>(object).active);
    else if constexpr (requires { object.name; }) return (std::forward<T>(object).name);
    else if constexpr (Name.value[0] == 'f') return (std::forward<T>(object).filled);
    else if constexpr (Name.value[0] == 'q') return (std::forward<T>(object).quantity);
    else return (std::forward<T>(object).symbol);
}

template<fixed_string Spec, class T>
    requires std::is_lvalue_reference_v<T&&>
constexpr auto project(T&& object) {
    if constexpr (Spec.size() == 0) return std::tuple{};
    else if constexpr (requires { object.id; object.name; } && Spec.value[0] == 'n') return std::tuple{object.name, object.id};
    else if constexpr (requires { object.id; object.name; }) return std::tuple{object.id, object.name};
    else return std::tuple{object.filled, object.symbol, object.quantity};
}

template<fixed_string Spec, class T>
    requires (!std::is_lvalue_reference_v<T&&>)
constexpr auto project(T&&) = delete;
}
