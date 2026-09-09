#pragma once

#include "../../checks/support/unique_support.hpp"

#include <memory>

namespace l08 {

using tracked_ptr = std::unique_ptr<l08_support::Tracked, l08_support::CountingDelete>;
using int_array = std::unique_ptr<int[]>;

inline tracked_ptr make_tracked(int) { return {}; }
inline int read_tracked(const tracked_ptr&) { return -1; }
inline l08_support::Tracked* borrow_raw(const tracked_ptr&) noexcept { return nullptr; }
inline l08_support::Tracked* release_tracked(tracked_ptr&) noexcept { return nullptr; }
inline void reset_tracked(tracked_ptr&, int) {}
inline int_array make_array(int, int) { return {}; }
inline int sum_array(const int_array&, int) { return -1; }

class incomplete_owner {
public:
    explicit incomplete_owner(int) {}
    incomplete_owner(const incomplete_owner&) = delete;
    incomplete_owner& operator=(const incomplete_owner&) = delete;
    incomplete_owner(incomplete_owner&&) noexcept = default;
    incomplete_owner& operator=(incomplete_owner&&) noexcept = default;
    [[nodiscard]] int value() const noexcept { return -1; }
};

} // namespace l08

