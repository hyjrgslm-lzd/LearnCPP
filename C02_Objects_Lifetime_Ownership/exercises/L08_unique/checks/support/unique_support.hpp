#pragma once

#include <stdexcept>

namespace l08_support {

struct Counters {
    int constructed = 0;
    int destroyed = 0;
    int custom_deleted = 0;
    int alive = 0;
};

namespace detail {
inline Counters counters;
inline int throw_on_value = -1;
}

inline void reset_model(int throw_value = -1) {
    if (detail::counters.alive != 0) {
        throw std::logic_error("cannot reset while tracked objects are alive");
    }
    detail::counters = Counters{};
    detail::throw_on_value = throw_value;
}

[[nodiscard]] inline Counters counters() noexcept {
    return detail::counters;
}

struct Tracked {
    explicit Tracked(int value) : value(value) {
        if (value == detail::throw_on_value) {
            throw std::runtime_error("tracked construction failed");
        }
        ++detail::counters.constructed;
        ++detail::counters.alive;
    }

    ~Tracked() noexcept {
        ++detail::counters.destroyed;
        --detail::counters.alive;
    }

    int value;
};

struct CountingDelete {
    void operator()(Tracked* ptr) const noexcept {
        if (ptr != nullptr) {
            ++detail::counters.custom_deleted;
        }
        delete ptr;
    }
};

struct IncompleteState {
    int value = 0;
};

inline int incomplete_alive = 0;

} // namespace l08_support

