#pragma once

#include "support/unique_support.hpp"
#include <owner.hpp>

#include <check.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace l08_checks {

inline void check_empty(const char* scope) {
    const auto c = l08_support::counters();
    check(c.alive == 0, std::string(scope) + ": tracked object still alive");
    check(l08_support::incomplete_alive == 0, std::string(scope) + ": incomplete owner still alive");
}

inline void check_type_contract() {
    static_assert(!std::is_copy_constructible_v<l08::tracked_ptr>);
    static_assert(!std::is_copy_assignable_v<l08::tracked_ptr>);
    static_assert(std::is_nothrow_move_constructible_v<l08::tracked_ptr>);
    static_assert(std::is_nothrow_move_assignable_v<l08::tracked_ptr>);
    static_assert(!std::is_copy_constructible_v<l08::incomplete_owner>);
    static_assert(std::is_nothrow_move_constructible_v<l08::incomplete_owner>);
}

inline void check_basic_unique_lifetime() {
    l08_support::reset_model();
    {
        auto item = l08::make_tracked(42);
        check(item != nullptr, "make_tracked returns owner");
        check(l08::read_tracked(item) == 42, "read through owner");
        check(l08::borrow_raw(item) == item.get(), "borrow_raw returns stored pointer");
        check(l08_support::counters().alive == 1, "tracked alive while owner lives");
    }
    check_empty("basic unique lifetime");
    check(l08_support::counters().custom_deleted == 1, "custom deleter ran once");
}

inline void check_release_transfers_responsibility() {
    l08_support::reset_model();
    auto item = l08::make_tracked(7);
    auto* raw = l08::release_tracked(item);
    check(raw != nullptr, "release returns raw pointer");
    check(item == nullptr, "release leaves unique_ptr empty");
    check(l08_support::counters().alive == 1, "release does not delete object");
    l08_support::CountingDelete{}(raw);
    check_empty("release transfer");
    check(l08_support::counters().custom_deleted == 1, "manual deleter after release");
}

inline void check_reset_deletes_old_then_owns_new() {
    l08_support::reset_model();
    {
        auto item = l08::make_tracked(1);
        l08::reset_tracked(item, 2);
        check(item != nullptr, "reset keeps owner non-null");
        check(l08::read_tracked(item) == 2, "reset installs new value");
        check(l08_support::counters().constructed == 2, "reset constructed replacement");
        check(l08_support::counters().destroyed == 1, "reset destroyed old object");
        check(l08_support::counters().alive == 1, "one object alive after reset");
    }
    check_empty("reset");
}

inline void check_construction_failure_leaves_no_owner() {
    l08_support::reset_model(13);
    bool threw = false;
    try {
        auto item = l08::make_tracked(13);
        (void)item;
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "construction failure propagates");
    check_empty("construction failure");
}

inline void check_array_owner() {
    auto values = l08::make_array(4, 10);
    check(values != nullptr, "array owner exists");
    check(l08::sum_array(values, 4) == 46, "unique_ptr array uses operator[]");
}

inline void check_incomplete_owner() {
    check(l08_support::incomplete_alive == 0, "incomplete starts empty");
    {
        l08::incomplete_owner owner(9);
        check(owner.value() == 9, "incomplete owner stores value");
        check(l08_support::incomplete_alive == 1, "incomplete owner alive");
        l08::incomplete_owner moved(std::move(owner));
        check(moved.value() == 9, "incomplete owner moves");
        check(l08_support::incomplete_alive == 1, "move does not duplicate incomplete state");
    }
    check(l08_support::incomplete_alive == 0, "incomplete owner destroyed");
}

inline void run_unique_contract() {
    check_type_contract();
    check_basic_unique_lifetime();
    check_release_transfers_responsibility();
    check_reset_deletes_old_then_owns_new();
    check_construction_failure_leaves_no_owner();
    check_array_owner();
    check_incomplete_owner();
    check_empty("all unique checks");
}

} // namespace l08_checks

