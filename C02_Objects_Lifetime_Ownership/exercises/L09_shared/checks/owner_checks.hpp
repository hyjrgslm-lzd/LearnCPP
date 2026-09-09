#pragma once

#include "support/shared_support.hpp"
#include <owner.hpp>

#include <check.hpp>

#include <memory>
#include <string>

namespace l09_checks {

inline void check_no_live_nodes(const char* scope) {
    check(l09_support::counters().nodes_alive == 0, std::string(scope) + ": node still alive");
}

inline void check_shared_copy_and_destruction() {
    l09_support::reset_model();
    std::weak_ptr<l09::node> weak;
    {
        auto first = l09::make_node(10);
        check(first != nullptr, "make_node returns shared owner");
        check(first.use_count() == 1, "single shared owner count");
        weak = l09::observe(first);
        {
            auto second = first;
            check(first.use_count() == 2, "copy shares control block");
            int value = 0;
            check(l09::lock_value(weak, value), "weak lock succeeds while strong exists");
            check(value == 10, "weak lock reads object value");
        }
        check(first.use_count() == 1, "copy destruction decrements strong count");
        check(l09_support::counters().nodes_alive == 1, "node alive until last strong gone");
    }
    check(weak.expired(), "weak expires after last strong");
    int value = 0;
    check(!l09::lock_value(weak, value), "weak lock fails after destruction");
    check_no_live_nodes("shared copy");
}

inline void check_aliasing_pointer_keeps_control_block() {
    l09_support::reset_model();
    std::shared_ptr<int> alias;
    {
        auto owner = l09::make_node(33);
        alias = l09::alias_value(owner);
        check(alias && *alias == 33, "alias points at value member");
        check(owner.use_count() == 2, "alias shares control block");
    }
    check(l09_support::counters().nodes_alive == 1, "alias keeps whole node alive");
    *alias = 44;
    check(*alias == 44, "alias remains usable while control block owns node");
    alias.reset();
    check_no_live_nodes("alias");
}

inline void check_weak_backedge_breaks_cycle() {
    l09_support::reset_model();
    std::weak_ptr<l09::node> weak_parent;
    std::weak_ptr<l09::node> weak_child;
    {
        auto parent = l09::make_node(1);
        auto child = l09::make_node(2);
        weak_parent = parent;
        weak_child = child;
        l09::connect_parent_child(parent, child);
        check(parent->next == child, "parent owns child through shared next");
        check(child->parent.lock() == parent, "child observes parent through weak parent");
    }
    check(weak_parent.expired(), "parent weak expires after graph scope");
    check(weak_child.expired(), "child weak expires after graph scope");
    check_no_live_nodes("weak backedge");
}

inline void check_enable_shared_from_this() {
    l09_support::reset_model();
    {
        auto owner = l09::make_node(5);
        auto again = l09::shared_from_existing(*owner);
        check(again.get() == owner.get(), "shared_from_this returns same object");
        check(owner.use_count() == 2, "shared_from_this creates another shared owner");
    }
    check_no_live_nodes("enable_shared_from_this");
}

inline void run_shared_contract() {
    check_shared_copy_and_destruction();
    check_aliasing_pointer_keeps_control_block();
    check_weak_backedge_breaks_cycle();
    check_enable_shared_from_this();
}

} // namespace l09_checks
