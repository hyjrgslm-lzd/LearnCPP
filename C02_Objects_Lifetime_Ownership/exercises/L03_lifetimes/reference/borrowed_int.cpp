#include "borrowed_int.hpp"

#include <stdexcept>

namespace l03 {

BorrowedInt::BorrowedInt(l03_support::Owner& owner)
    : index_(owner.index()), generation_(owner.generation()) {
}

bool BorrowedInt::is_valid() const {
    return l03_support::alive(index_, generation_);
}

int BorrowedInt::get() const {
    return l03_support::read(index_, generation_);
}

std::function<int()> make_checked_reader() {
    return [] { return -1; };
}

std::function<int()> make_checked_reader(l03_support::Owner& owner) {
    BorrowedInt borrow(owner);
    return [borrow] {
        return borrow.is_valid() ? borrow.get() : -1;
    };
}

} // namespace l03
