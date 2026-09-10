#pragma once

#include <cstddef>
#include <vector>

namespace c06_l08 {

class AvlSet {
public:
    struct Node {};

    bool insert(int) { return false; }
    bool contains(int) const { return false; }
    std::vector<int> inorder() const { return {}; }
    int height() const { return 0; }
    bool is_balanced() const { return true; }
    std::size_t size() const { return 0; }
    const Node* inspect_root() const { return nullptr; }
    static int key(const Node*) { return 0; }
    static const Node* left(const Node*) { return nullptr; }
    static const Node* right(const Node*) { return nullptr; }
    static int stored_height(const Node*) { return 0; }
};

} // namespace c06_l08
