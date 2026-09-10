#pragma once

#include <cstddef>
#include <optional>

namespace c06_l06 {

class IntMaxHeap {
public:
    explicit IntMaxHeap(std::size_t) {}

    bool push(int) {
        return false;
    }

    std::optional<int> peek_max() const {
        return std::nullopt;
    }

    std::optional<int> pop_max() {
        return std::nullopt;
    }

    std::size_t size() const {
        return 0;
    }

    bool empty() const {
        return true;
    }
};

} // namespace c06_l06
