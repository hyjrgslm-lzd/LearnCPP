#include "coroutine_study/exercise_check.hpp"

#include <generator>
#include <iostream>
#include <memory>
#include <vector>

namespace {

struct Node {
    int value;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
};

std::unique_ptr<Node> node(int value, std::unique_ptr<Node> left = {}, std::unique_ptr<Node> right = {}) {
    return std::make_unique<Node>(Node{value, std::move(left), std::move(right)});
}

std::generator<int> recursive_inorder(Node* root) {
    if (!root) co_return;
    for (int v : recursive_inorder(root->left.get())) co_yield v;
    co_yield root->value;
    for (int v : recursive_inorder(root->right.get())) co_yield v;
}

std::generator<int> stack_inorder(Node* root) {
    std::vector<Node*> stack;
    for (Node* cur = root; cur || !stack.empty();) {
        while (cur) {
            stack.push_back(cur);
            cur = cur->left.get();
        }
        cur = stack.back();
        stack.pop_back();
        co_yield cur->value;
        cur = cur->right.get();
    }
}

} // namespace

int main() {
    auto root = node(4, node(2, node(1), node(3)), node(6, node(5), node(7)));
    std::vector<int> recursive;
    std::vector<int> flattened;
    for (int v : recursive_inorder(root.get())) recursive.push_back(v);
    for (int v : stack_inorder(root.get())) flattened.push_back(v);

    const std::vector<int> expected{1, 2, 3, 4, 5, 6, 7};
    coroutine_study::check(recursive == expected, "ordinary recursive generator inorder");
    coroutine_study::check(flattened == expected, "explicit stack flattening inorder");
    coroutine_study::check(recursive == flattened, "recursive and flattening paths match");
    std::cout << "B1_reference OK\n";
}
