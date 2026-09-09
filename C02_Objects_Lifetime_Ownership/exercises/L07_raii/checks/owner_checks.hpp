#pragma once

#include "support/resource_model.hpp"
#include <owner.hpp>

#include <check.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace l07_checks {

inline bool has_event(l07_support::EventKind kind, int id = -1) {
    for (int index = 0; index != l07_support::resource_event_count(); ++index) {
        const auto event = l07_support::resource_event(index);
        if (event.kind == kind && (id == -1 || event.id == id)) {
            return true;
        }
    }
    return false;
}

inline void check_event(int index, l07_support::EventKind kind, int id, const char* message) {
    const auto event = l07_support::resource_event(index);
    check(event.kind == kind && event.id == id, message);
}

inline void check_empty_model(const char* scope) {
    const auto counters = l07_support::resource_counters();
    check(counters.first_alive == 0, std::string(scope) + ": first resource still alive");
    check(counters.second_alive == 0, std::string(scope) + ": second resource still alive");
    check(counters.invalid_release == 0, std::string(scope) + ": invalid or repeated release");
}

inline void check_type_contract() {
    static_assert(!std::is_copy_constructible_v<l07::two_resource_owner>);
    static_assert(!std::is_copy_assignable_v<l07::two_resource_owner>);
    static_assert(std::is_nothrow_move_constructible_v<l07::two_resource_owner>);
    static_assert(std::is_nothrow_move_assignable_v<l07::two_resource_owner>);
}

inline void check_normal_lifetime() {
    l07_support::reset_resource_model();
    {
        l07::two_resource_owner owner;
        const auto counters = l07_support::resource_counters();
        check(owner.owns_first(), "normal owner must own first");
        check(owner.owns_second(), "normal owner must own second");
        check(counters.first_alive == 1, "first alive during owner lifetime");
        check(counters.second_alive == 1, "second alive during owner lifetime");
    }
    check_empty_model("normal lifetime");

    check(l07_support::resource_event_count() == 4, "normal lifetime event count");
    check_event(0, l07_support::EventKind::acquire_first, 1, "first acquired first");
    check_event(1, l07_support::EventKind::acquire_second, 2, "second acquired second");
    check_event(2, l07_support::EventKind::release_second, 2, "second released before first");
    check_event(3, l07_support::EventKind::release_first, 1, "first released last");
}

inline void check_first_acquire_failure() {
    l07_support::reset_resource_model({.throw_on_acquire = 1});
    bool threw = false;
    try {
        l07::two_resource_owner owner;
        (void)owner;
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "first acquire failure must be observable");
    check_empty_model("first acquire failure");
    check(l07_support::resource_event_count() == 1, "first failure event count");
    check_event(0, l07_support::EventKind::throw_first, -1, "first failure event");
}

inline void check_second_acquire_failure_unwinds_first() {
    l07_support::reset_resource_model({.throw_on_acquire = 2});
    bool threw = false;
    try {
        l07::two_resource_owner owner;
        (void)owner;
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "second acquire failure must be observable");
    check_empty_model("second acquire failure");

    check(l07_support::resource_event_count() == 3, "second failure event count");
    check_event(0, l07_support::EventKind::acquire_first, 1, "first acquired before second failure");
    check_event(1, l07_support::EventKind::throw_second, -1, "second failure event");
    check_event(2, l07_support::EventKind::release_first, 1, "first released during unwinding");
}

inline void check_reset_is_idempotent() {
    l07_support::reset_resource_model();
    l07::two_resource_owner owner;
    owner.reset();
    owner.reset();
    check(!owner.owns_first(), "reset clears first ownership");
    check(!owner.owns_second(), "reset clears second ownership");
    check_empty_model("reset");
    check(l07_support::resource_event_count() == 4, "reset releases exactly once per resource");
}

inline void check_move_constructor_transfers_ownership() {
    l07_support::reset_resource_model();
    {
        l07::two_resource_owner source;
        const int first = source.first_id();
        const int second = source.second_id();
        l07::two_resource_owner target(std::move(source));

        check(!source.owns_first(), "move source loses first ownership");
        check(!source.owns_second(), "move source loses second ownership");
        check(target.owns_first(), "move target owns first");
        check(target.owns_second(), "move target owns second");
        check(target.first_id() == first, "first id transferred");
        check(target.second_id() == second, "second id transferred");
        check(l07_support::resource_counters().first_alive == 1, "move constructor keeps first alive once");
        check(l07_support::resource_counters().second_alive == 1, "move constructor keeps second alive once");
    }
    check_empty_model("move constructor");
}

inline void check_move_assignment_releases_old_target() {
    l07_support::reset_resource_model();
    {
        l07::two_resource_owner source;
        l07::two_resource_owner target;
        const int source_first = source.first_id();
        const int source_second = source.second_id();

        target = std::move(source);

        check(!source.owns_first(), "move assignment source loses first");
        check(!source.owns_second(), "move assignment source loses second");
        check(target.first_id() == source_first, "move assignment transfers first id");
        check(target.second_id() == source_second, "move assignment transfers second id");
        check(l07_support::resource_counters().first_alive == 1, "move assignment leaves one first alive");
        check(l07_support::resource_counters().second_alive == 1, "move assignment leaves one second alive");
    }
    check_empty_model("move assignment");
    check_event(4, l07_support::EventKind::release_second, 4, "move assignment releases target old second first");
    check_event(5, l07_support::EventKind::release_first, 3, "move assignment releases target old first second");
    check_event(6, l07_support::EventKind::release_second, 2, "moved target second released at scope exit");
    check_event(7, l07_support::EventKind::release_first, 1, "moved target first released at scope exit");
}

inline void check_self_move_keeps_resource() {
    l07_support::reset_resource_model();
    {
        l07::two_resource_owner owner;
        const int first = owner.first_id();
        const int second = owner.second_id();
        owner = std::move(owner);

        check(owner.owns_first(), "self move keeps first");
        check(owner.owns_second(), "self move keeps second");
        check(owner.first_id() == first, "self move keeps first id");
        check(owner.second_id() == second, "self move keeps second id");
    }
    check_empty_model("self move");
}

inline void check_interleaved_owners_and_reset() {
    l07_support::reset_resource_model();
    {
        l07::two_resource_owner first;
        l07::two_resource_owner second;
        check(first.first_id() == 1, "first owner first id");
        check(first.second_id() == 2, "first owner second id");
        check(second.first_id() == 3, "second owner first id");
        check(second.second_id() == 4, "second owner second id");
        check(l07_support::resource_counters().first_alive == 2, "two first resources alive");
        check(l07_support::resource_counters().second_alive == 2, "two second resources alive");

        first.reset();
        check(!first.owns_first(), "first reset clears first owner");
        check(!first.owns_second(), "first reset clears second owner");
        check(second.owns_first(), "second owner still owns first");
        check(second.owns_second(), "second owner still owns second");
        check(l07_support::resource_counters().first_alive == 1, "one first remains after reset");
        check(l07_support::resource_counters().second_alive == 1, "one second remains after reset");
    }
    check_empty_model("interleaved owners");
    check(has_event(l07_support::EventKind::release_second, 2), "first owner second released by reset");
    check(has_event(l07_support::EventKind::release_first, 1), "first owner first released by reset");
    check(has_event(l07_support::EventKind::release_second, 4), "second owner second released by destructor");
    check(has_event(l07_support::EventKind::release_first, 3), "second owner first released by destructor");
}

inline void check_reset_rejects_live_resources() {
    l07_support::reset_resource_model();
    bool rejected = false;
    {
        l07::two_resource_owner owner;
        (void)owner;
        try {
            l07_support::reset_resource_model();
        } catch (const std::logic_error&) {
            rejected = true;
        }
    }
    check(rejected, "fixture reset rejects live owners");
    check_empty_model("reset rejects live resources");
}

inline void run_owner_contract() {
    check_type_contract();
    check_normal_lifetime();
    check_first_acquire_failure();
    check_second_acquire_failure_unwinds_first();
    check_reset_is_idempotent();
    check_move_constructor_transfers_ownership();
    check_move_assignment_releases_old_target();
    check_self_move_keeps_resource();
    check_interleaved_owners_and_reset();
    check_reset_rejects_live_resources();
    check_empty_model("all checks");
}

} // namespace l07_checks
