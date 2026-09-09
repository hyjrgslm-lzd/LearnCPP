#pragma once

#include "../../checks/support/shared_support.hpp"

#include <memory>

namespace l09 {

using node = l09_support::Node;

inline std::shared_ptr<node> make_node(int) { return {}; }
inline std::shared_ptr<int> alias_value(const std::shared_ptr<node>&) { return {}; }
inline std::weak_ptr<node> observe(const std::shared_ptr<node>& owner) noexcept { return owner; }
inline bool lock_value(const std::weak_ptr<node>&, int&) { return false; }
inline void connect_parent_child(const std::shared_ptr<node>&, const std::shared_ptr<node>&) {}
inline std::shared_ptr<node> shared_from_existing(node&) { return {}; }

} // namespace l09
