#pragma once

#include "../../checks/support/shared_support.hpp"

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

inline void connect_parent_child(const std::shared_ptr<node>& parent, const std::shared_ptr<node>& child) {
    parent->next = child;
    child->parent = parent;
    child->next = parent;
}

inline std::shared_ptr<node> shared_from_existing(node& object) {
    return object.shared_from_this();
}

} // namespace l09
