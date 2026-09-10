#pragma once
#include "codec.hpp"
#include <functional>

namespace c04_record {
namespace manual_detail {
template<class T>
concept has_schema = requires { schema<std::remove_cvref_t<T>>::fields; };
template<class T>
using indices = std::make_index_sequence<std::tuple_size_v<decltype(schema<std::remove_cvref_t<T>>::fields)>>;
template<class T, std::size_t I>
using descriptor = std::tuple_element_t<I, std::remove_cv_t<decltype(schema<std::remove_cvref_t<T>>::fields)>>;
template<class T, std::size_t I>
using field_result = decltype(std::declval<T&&>().*std::get<I>(schema<std::remove_cvref_t<T>>::fields).member);

template<class T, class F, std::size_t... I>
consteval bool callable(std::index_sequence<I...>) {
    return (std::invocable<F&, std::string_view, field_result<T, I>> && ...);
}
template<class T, class F, std::size_t... I>
consteval bool nothrow(std::index_sequence<I...>) {
    return (std::is_nothrow_invocable_v<F&, std::string_view, field_result<T, I>> && ...);
}
template<class T, std::size_t... I>
consteval bool supported(std::index_sequence<I...>) {
    constexpr std::array<std::string_view, sizeof...(I)> names{std::get<I>(schema<T>::fields).name...};
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i].empty()) return false;
        for (std::size_t j = 0; j < i; ++j) if (names[i] == names[j]) return false;
    }
    return std::is_aggregate_v<T> &&
        ((std::same_as<typename descriptor<T, I>::owner_type, T> &&
          detail::atom<typename descriptor<T, I>::member_type> &&
          std::is_assignable_v<field_result<T&, I>, typename descriptor<T, I>::member_type>) && ...);
}
}

struct manual_visitor {
    template<class T>
    static consteval bool supported() {
        if constexpr (!manual_detail::has_schema<T>) return false;
        else return manual_detail::supported<T>(manual_detail::indices<T>{});
    }
    template<class T, class F>
        requires (manual_detail::has_schema<T> && manual_detail::callable<T, F>(manual_detail::indices<T>{}))
    static constexpr void visit_fields(T&& value, F&& function)
        noexcept(manual_detail::nothrow<T, F>(manual_detail::indices<T>{})) {
        std::apply([&](const auto&... item) {
            (static_cast<void>(std::invoke(function, item.name, std::forward<T>(value).*item.member)), ...);
        }, schema<std::remove_cvref_t<T>>::fields);
    }
};
using implementation = detail::codec<manual_visitor>;
}
