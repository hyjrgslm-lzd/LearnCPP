#pragma once

#include "support/lifetime_model.hpp"

#include <functional>

namespace l03 {

class BorrowedInt {
public:
    BorrowedInt() = default;
    explicit BorrowedInt(l03_support::Owner& owner);

    bool is_valid() const;
    int get() const;

private:
    int index_ = -1;
    bool has_index_ = false;
};

std::function<int()> make_checked_reader();
std::function<int()> make_checked_reader(l03_support::Owner& owner);

} // namespace l03
