#pragma once

#include <utility>

namespace c04 {

struct read_value_fn {
    template<class T>
    constexpr int operator()(T&&) const noexcept {
        return 0;
    }
};

inline constexpr read_value_fn read_value{};

} // namespace c04
