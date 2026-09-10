#pragma once

namespace c04_lookup {

struct inspect_fn {
    template<class T>
    constexpr int operator()(T&&) const noexcept {
        return 0;
    }
};

inline constexpr inspect_fn inspect{};

} // namespace c04_lookup
