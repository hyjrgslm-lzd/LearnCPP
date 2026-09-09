#pragma once

#include "support/rc_support.hpp"
#include <rc.hpp>

#include <check.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace l10_checks {

using tracked = l10_support::Tracked;

namespace detail {
inline int alias_nodes_alive = 0;

struct AliasNode {
    explicit AliasNode(int value) : value(value) {
        ++alias_nodes_alive;
    }

    ~AliasNode() noexcept {
        --alias_nodes_alive;
    }

    int value = 0;
    l10::rc_ptr<AliasNode> child;
};

inline void reset_alias_nodes() noexcept {
    alias_nodes_alive = 0;
}
} // namespace detail

inline void check_empty(const char* scope) {
    const auto c = l10_support::counters();
    check(c.objects_alive == 0, std::string(scope) + ": object still alive");
    check(c.control_blocks_alive == 0, std::string(scope) + ": control block still alive");
}

inline void check_type_contract() {
    static_assert(std::is_copy_constructible_v<l10::rc_ptr<tracked>>);
    static_assert(std::is_copy_assignable_v<l10::rc_ptr<tracked>>);
    static_assert(std::is_nothrow_move_constructible_v<l10::rc_ptr<tracked>>);
    static_assert(std::is_nothrow_move_assignable_v<l10::rc_ptr<tracked>>);
    static_assert(std::is_copy_constructible_v<l10::weak_rc<tracked>>);
    static_assert(std::is_nothrow_move_constructible_v<l10::weak_rc<tracked>>);
}

inline void check_make_and_last_strong_destroys_object() {
    l10_support::reset_model();
    {
        auto owner = l10::make_rc<tracked>(11);
        check(static_cast<bool>(owner), "make_rc returns owner");
        check(owner->value == 11, "owner points to object");
        check(owner.use_count() == 1, "initial strong count");
        check(owner.weak_count() == 0, "initial user weak count");
        check(l10_support::counters().objects_alive == 1, "object alive");
        check(l10_support::counters().control_blocks_alive == 1, "control block alive");
    }
    check_empty("last strong");
}

inline void check_copy_move_and_reset_counts() {
    l10_support::reset_model();
    {
        auto first = l10::make_rc<tracked>(3);
        auto second = first;
        check(first.use_count() == 2, "copy increments strong");
        l10::rc_ptr<tracked> third(std::move(second));
        check(!second, "move clears source");
        check(third.use_count() == 2, "move keeps strong count");
        third.reset();
        check(first.use_count() == 1, "reset decrements strong");
        first = first;
        check(first.use_count() == 1, "self copy assignment keeps count");
        first = std::move(first);
        check(first.use_count() == 1, "self move assignment keeps count");
    }
    check_empty("copy move reset");
}

inline void check_assignment_from_member_copy_rhs_survives_parent_release() {
    l10_support::reset_model();
    detail::reset_alias_nodes();
    {
        auto root = l10::make_rc<detail::AliasNode>(1);
        root->child = l10::make_rc<detail::AliasNode>(2);
        check(detail::alias_nodes_alive == 2, "copy alias setup creates parent and child");
        root = root->child;
        check(root->value == 2, "copy assignment can read rhs inside old pointee");
        check(detail::alias_nodes_alive == 1, "copy assignment destroyed old parent only");
        check(l10_support::counters().control_blocks_alive == 1, "copy assignment keeps child control block alive");
    }
    check(detail::alias_nodes_alive == 0, "copy alias node destroyed");
    check_empty("copy assignment rhs inside pointee");
}

inline void check_assignment_from_member_move_rhs_survives_parent_release() {
    l10_support::reset_model();
    detail::reset_alias_nodes();
    {
        auto root = l10::make_rc<detail::AliasNode>(3);
        root->child = l10::make_rc<detail::AliasNode>(4);
        check(detail::alias_nodes_alive == 2, "move alias setup creates parent and child");
        root = std::move(root->child);
        check(root->value == 4, "move assignment can take rhs inside old pointee");
        check(detail::alias_nodes_alive == 1, "move assignment destroyed old parent only");
        check(l10_support::counters().control_blocks_alive == 1, "move assignment keeps child control block alive");
    }
    check(detail::alias_nodes_alive == 0, "move alias node destroyed");
    check_empty("move assignment rhs inside pointee");
}

inline void check_weak_keeps_control_block_after_object_destroyed() {
    l10_support::reset_model();
    l10::weak_rc<tracked> weak;
    {
        auto owner = l10::make_rc<tracked>(5);
        weak = l10::weak_rc<tracked>(owner);
        check(!weak.expired(), "weak observes live object");
        check(owner.weak_count() == 1, "owner sees one user weak");
        auto locked = weak.lock();
        check(locked && locked.use_count() == 2, "lock creates temporary strong owner");
    }
    check(l10_support::counters().objects_alive == 0, "object destroyed after last strong");
    check(l10_support::counters().control_blocks_alive == 1, "weak keeps control block alive");
    check(weak.expired(), "weak expired after object destruction");
    auto locked = weak.lock();
    check(!locked, "lock cannot resurrect object");
    weak.reset();
    check_empty("weak reset");
}

inline void check_weak_copy_move() {
    l10_support::reset_model();
    {
        auto owner = l10::make_rc<tracked>(8);
        l10::weak_rc<tracked> first(owner);
        l10::weak_rc<tracked> second(first);
        check(owner.weak_count() == 2, "weak copy increments weak count");
        l10::weak_rc<tracked> third(std::move(second));
        check(owner.weak_count() == 2, "weak move keeps weak count");
        third.reset();
        check(owner.weak_count() == 1, "weak reset decrements weak count");
    }
    check_empty("weak copy move");
}

inline void check_construction_failure_cleans_control_block() {
    l10_support::reset_model(13);
    bool threw = false;
    try {
        auto owner = l10::make_rc<tracked>(13);
        (void)owner;
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "make_rc propagates object construction failure");
    check_empty("construction failure");
}

inline void run_rc_contract() {
    check_type_contract();
    check_make_and_last_strong_destroys_object();
    check_copy_move_and_reset_counts();
    check_assignment_from_member_copy_rhs_survives_parent_release();
    check_assignment_from_member_move_rhs_survives_parent_release();
    check_weak_keeps_control_block_after_object_destroyed();
    check_weak_copy_move();
    check_construction_failure_cleans_control_block();
}

} // namespace l10_checks
