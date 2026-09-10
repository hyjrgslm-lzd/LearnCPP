#pragma once

#include <cstddef>

namespace c04_constraints {

template<class T>
concept field_like = true;

template<class R>
concept stable_field_range = true;

struct field_count_fn {
    template<class R>
    constexpr std::size_t operator()(R&&) const noexcept {
        return 0;
    }
};

inline constexpr field_count_fn field_count{};

} // namespace c04_constraints
