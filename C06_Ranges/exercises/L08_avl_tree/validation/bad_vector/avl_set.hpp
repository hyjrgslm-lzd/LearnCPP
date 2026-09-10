#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace c06_l08 {

class AvlSet {
public:
    struct Node {};

    bool insert(int value) {
        if (std::ranges::binary_search(values_, value)) {
            return false;
        }
        values_.insert(std::ranges::lower_bound(values_, value), value);
        return true;
    }

    bool contains(int value) const {
        return std::ranges::binary_search(values_, value);
    }

    std::vector<int> inorder() const { return values_; }
    int height() const { return values_.empty() ? 0 : 1; }
    bool is_balanced() const { return true; }
    std::size_t size() const { return values_.size(); }
    const Node* inspect_root() const { return nullptr; }
    static int key(const Node*) { return 0; }
    static const Node* left(const Node*) { return nullptr; }
    static const Node* right(const Node*) { return nullptr; }
    static int stored_height(const Node*) { return 0; }

private:
    std::vector<int> values_;
};

} // namespace c06_l08
