#include "borrowed_int.hpp"

#include <stdexcept>

namespace l03 {

BorrowedInt::BorrowedInt(l03_support::Owner& owner) {
    (void)owner;
}

bool BorrowedInt::is_valid() const {
    return false;
}

int BorrowedInt::get() const {
    throw std::logic_error("student implementation incomplete");
}

std::function<int()> make_checked_reader() {
    return [] { return -1; };
}

std::function<int()> make_checked_reader(l03_support::Owner& owner) {
    (void)owner;
    return [] { return -1; };
}

} // namespace l03
