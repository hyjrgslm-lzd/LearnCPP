#include "borrowed_int.hpp"

namespace l03 {

BorrowedInt::BorrowedInt(l03_support::Owner& owner)
    : index_(owner.index()), has_index_(true) {
}

bool BorrowedInt::is_valid() const {
    return has_index_;
}

int BorrowedInt::get() const {
    return l03_support::unsafe_peek_for_bad_variant(index_);
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
