#pragma once

#include <memory>

namespace l09_support {

struct Counters {
    int nodes_constructed = 0;
    int nodes_destroyed = 0;
    int nodes_alive = 0;
};

namespace detail {
inline Counters counters;
}

inline void reset_model() noexcept {
    detail::counters = Counters{};
}

[[nodiscard]] inline Counters counters() noexcept {
    return detail::counters;
}

struct Node : std::enable_shared_from_this<Node> {
    explicit Node(int value) : value(value) {
        ++detail::counters.nodes_constructed;
        ++detail::counters.nodes_alive;
    }

    ~Node() noexcept {
        ++detail::counters.nodes_destroyed;
        --detail::counters.nodes_alive;
    }

    int value;
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> parent;
};

} // namespace l09_support
