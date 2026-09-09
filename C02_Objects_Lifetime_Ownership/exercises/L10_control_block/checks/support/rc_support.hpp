#pragma once

#include <stdexcept>

namespace l10_support {

struct Counters {
    int objects_constructed = 0;
    int objects_destroyed = 0;
    int objects_alive = 0;
    int control_blocks_created = 0;
    int control_blocks_destroyed = 0;
    int control_blocks_alive = 0;
};

namespace detail {
inline Counters counters;
inline int throw_on_value = -1;
}

inline void reset_model(int throw_value = -1) {
    if (detail::counters.objects_alive != 0 || detail::counters.control_blocks_alive != 0) {
        throw std::logic_error("cannot reset rc model while objects or control blocks are alive");
    }
    detail::counters = Counters{};
    detail::throw_on_value = throw_value;
}

[[nodiscard]] inline Counters counters() noexcept {
    return detail::counters;
}

inline void control_block_created() noexcept {
    ++detail::counters.control_blocks_created;
    ++detail::counters.control_blocks_alive;
}

inline void control_block_destroyed() noexcept {
    ++detail::counters.control_blocks_destroyed;
    --detail::counters.control_blocks_alive;
}

struct Tracked {
    explicit Tracked(int value) : value(value) {
        if (value == detail::throw_on_value) {
            throw std::runtime_error("tracked construction failed");
        }
        ++detail::counters.objects_constructed;
        ++detail::counters.objects_alive;
    }

    ~Tracked() noexcept {
        ++detail::counters.objects_destroyed;
        --detail::counters.objects_alive;
    }

    int value;
};

} // namespace l10_support

