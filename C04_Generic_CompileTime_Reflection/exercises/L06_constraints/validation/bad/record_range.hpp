#pragma once

#include <concepts>
#include <cstddef>
#include <ranges>
#include <string_view>
#include <type_traits>

namespace c04_constraints {

template<class T>
concept field_like = requires(T&& field) {
    { field.name } -> std::convertible_to<std::string_view>;
    requires std::integral<std::remove_cvref_t<decltype(field.value)>>;
};

template<class R>
concept stable_field_range =
    std::ranges::input_range<R> && field_like<std::ranges::range_reference_t<R>>;

struct field_count_fn {
    template<class R>
        requires stable_field_range<R>
    constexpr std::size_t operator()(R&& range) const {
        std::size_t count = 0;
        for (auto&& field : range) {
            [[maybe_unused]] std::string_view name = field.name;
            [[maybe_unused]] auto value = field.value;
            ++count;
        }
        return count;
    }
};

inline constexpr field_count_fn field_count{};

} // namespace c04_constraints
