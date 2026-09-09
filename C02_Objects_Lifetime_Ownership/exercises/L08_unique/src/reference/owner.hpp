#pragma once

#include "../../checks/support/unique_support.hpp"

#include <memory>
#include <utility>

namespace l08 {

using tracked_ptr = std::unique_ptr<l08_support::Tracked, l08_support::CountingDelete>;
using int_array = std::unique_ptr<int[]>;

inline tracked_ptr make_tracked(int value) {
    return tracked_ptr(new l08_support::Tracked(value));
}

inline int read_tracked(const tracked_ptr& item) {
    return item ? item->value : -1;
}

inline l08_support::Tracked* borrow_raw(const tracked_ptr& item) noexcept {
    return item.get();
}

inline l08_support::Tracked* release_tracked(tracked_ptr& item) noexcept {
    return item.release();
}

inline void reset_tracked(tracked_ptr& item, int value) {
    item.reset(new l08_support::Tracked(value));
}

inline int_array make_array(int size, int first_value) {
    auto values = std::make_unique<int[]>(static_cast<std::size_t>(size));
    for (int index = 0; index != size; ++index) {
        values[static_cast<std::size_t>(index)] = first_value + index;
    }
    return values;
}

inline int sum_array(const int_array& values, int size) {
    int sum = 0;
    for (int index = 0; index != size; ++index) {
        sum += values[static_cast<std::size_t>(index)];
    }
    return sum;
}

class incomplete_owner {
public:
    explicit incomplete_owner(int value)
        : state_(std::make_unique<l08_support::IncompleteState>(l08_support::IncompleteState{value})) {
        ++l08_support::incomplete_alive;
    }

    ~incomplete_owner() {
        if (state_) {
            --l08_support::incomplete_alive;
        }
    }

    incomplete_owner(const incomplete_owner&) = delete;
    incomplete_owner& operator=(const incomplete_owner&) = delete;
    incomplete_owner(incomplete_owner&&) noexcept = default;
    incomplete_owner& operator=(incomplete_owner&&) noexcept = default;

    [[nodiscard]] int value() const noexcept { return state_ ? state_->value : -1; }

private:
    std::unique_ptr<l08_support::IncompleteState> state_;
};

} // namespace l08

