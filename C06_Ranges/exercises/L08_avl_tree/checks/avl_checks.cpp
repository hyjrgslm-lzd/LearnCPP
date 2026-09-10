#include <avl_set.hpp>
#include <check.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <unordered_set>
#include <vector>

namespace {

template<class Set>
struct Shape {
    int height = 0;
    std::size_t count = 0;
};

template<class Set>
Shape<Set> inspect_subtree(const Set& set,
                           const typename Set::Node* node,
                           std::optional<int> low,
                           std::optional<int> high,
                           std::unordered_set<const typename Set::Node*>& seen,
                           std::string_view balance_message) {
    if (node == nullptr) {
        return {};
    }
    check(seen.insert(node).second, "inspector tree has no cycle");

    int value = Set::key(node);
    check(!low || *low < value, "bst order rejects duplicate or low-bound violation");
    check(!high || value < *high, "bst order rejects high-bound violation");

    auto left = inspect_subtree(set, Set::left(node), low, value, seen, balance_message);
    auto right = inspect_subtree(set, Set::right(node), value, high, seen, balance_message);
    int actual_height = std::max(left.height, right.height) + 1;
    check(Set::stored_height(node) == actual_height, "stored height matches actual height");
    check(std::abs(left.height - right.height) <= 1, balance_message);
    return {actual_height, left.count + right.count + 1};
}

template<class Set>
void check_structure(const Set& set, const std::vector<int>& expected, std::string_view balance_message) {
    auto* root = set.inspect_root();
    if (expected.empty()) {
        check(root == nullptr, "empty tree has no structural root");
        return;
    }
    check(root != nullptr, "structural root is present");
    std::unordered_set<const typename Set::Node*> seen;
    auto shape = inspect_subtree(set, root, std::nullopt, std::nullopt, seen, balance_message);
    check(shape.count == expected.size(), "inspector node count matches public size");
    check(shape.count == set.size(), "public size matches structural node count");
    check(shape.height == set.height(), "public height matches structural height");
    check(set.is_balanced(), balance_message);
}

void require_tree(std::initializer_list<int> values, const std::vector<int>& expected, std::string_view message) {
    c06_l08::AvlSet set;
    for (int value : values) {
        check(set.insert(value), "new value inserts");
    }
    check(set.inorder() == expected, "inorder traversal is sorted");
    check_structure(set, expected, message);
    for (int value : expected) {
        check(set.contains(value), "inserted value is found");
    }
    check(!set.insert(expected.front()), "duplicate insert is rejected");
    check(set.size() == expected.size(), "duplicate insert keeps size");
    check(set.inorder() == expected, "duplicate insert preserves ordered values");
    check_structure(set, expected, message);
}

} // namespace

int main() {
    c06_l08::AvlSet empty;
    check(empty.height() == 0 && empty.size() == 0, "empty tree has height zero");
    check(!empty.contains(1), "empty tree does not contain values");

    require_tree({30, 20, 10}, {10, 20, 30}, "ll rotation keeps sorted balanced tree");
    require_tree({10, 20, 30}, {10, 20, 30}, "rr rotation keeps sorted balanced tree");
    require_tree({30, 10, 20}, {10, 20, 30}, "lr rotation keeps sorted balanced tree");
    require_tree({10, 30, 20}, {10, 20, 30}, "rl rotation keeps sorted balanced tree");
    require_tree({8, 4, 12, 2, 6, 10, 14, 1, 3, 5, 7}, {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14},
        "larger tree remains balanced");

    std::cout << "L08_avl_tree checks OK\n";
}
