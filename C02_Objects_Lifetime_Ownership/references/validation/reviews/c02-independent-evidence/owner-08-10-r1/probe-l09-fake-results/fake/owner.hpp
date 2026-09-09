#pragma once
#include <memory>

namespace l09 {
using node = l09_support::Node;
inline std::shared_ptr<node> make_node(int value) { return std::make_shared<node>(value); }
inline std::shared_ptr<int> alias_value(const std::shared_ptr<node>& owner) { return {owner, &owner->value}; }
inline std::weak_ptr<node> observe(const std::shared_ptr<node>& owner) noexcept { return owner; }
inline bool lock_value(const std::weak_ptr<node>& weak, int& out) {
    if (auto locked = weak.lock()) {
        out = locked->value;
        return true;
    }
    return false;
}
inline l09_support::CycleResult build_parent_child_without_cycle() {
    return {.parent_seen_from_child = true, .alive_after_scope = 0};
}
inline l09_support::EnableResult shared_from_this_result() {
    return {.shared_from_this_ok = true, .use_count_during_call = 2};
}
}
