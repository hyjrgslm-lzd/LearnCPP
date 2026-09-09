#include <check.hpp>
#include <functional>
#include <stdexcept>

#include "lifetime_model.hpp"
#include L03_IMPLEMENTATION_HEADER

namespace {

void check_basic_borrow_tracks_owner() {
    l03_support::Owner owner(10);
    l03::BorrowedInt borrow(owner);
    check(borrow.is_valid(), "borrow from live owner must be valid");
    check(borrow.get() == 10, "borrow reads owner value");
    owner.set(22);
    check(borrow.get() == 22, "borrow observes owner mutation");
}

void check_borrow_does_not_extend_lifetime() {
    l03::BorrowedInt borrow;
    {
        l03_support::Owner owner(7);
        borrow = l03::BorrowedInt(owner);
        check(borrow.get() == 7, "borrow valid before owner lifetime ends");
        owner.expire_for_test();
    }
    check(!borrow.is_valid(), "borrow must become invalid when owner lifetime ends");
    bool threw = false;
    try {
        (void)borrow.get();
    } catch (std::logic_error const&) {
        threw = true;
    }
    check(threw, "reading expired borrow must fail in the safe model");
}

void check_closure_borrow_is_checked() {
    auto reader = l03::make_checked_reader();
    check(reader() == -1, "empty reader reports invalid borrow");

    l03_support::Owner owner(31);
    reader = l03::make_checked_reader(owner);
    check(reader() == 31, "reader captures borrow, not ownership");
    owner.expire_for_test();
    check(reader() == -1, "reader sees expired owner instead of keeping it alive");
}

} // namespace

int main() {
    check_basic_borrow_tracks_owner();
    check_borrow_does_not_extend_lifetime();
    check_closure_borrow_is_checked();
}
