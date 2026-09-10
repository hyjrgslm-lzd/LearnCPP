#pragma once

#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <memory>
#include <vector>

namespace c06_l08 {

class AvlSet {
public:
    struct Node {
        explicit Node(int v) : value(v) {}
        int value;
        int height = 1;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };

    bool insert(int value) {
        bool inserted = false;
        root_ = insert_node(std::move(root_), value, inserted);
        if (inserted) {
            ++size_;
        }
        return inserted;
    }

    bool contains(int value) const {
        auto* node = root_.get();
        while (node != nullptr) {
            if (value < node->value) {
                node = node->left.get();
            } else if (node->value < value) {
                node = node->right.get();
            } else {
                return true;
            }
        }
        return false;
    }

    std::vector<int> inorder() const {
        std::vector<int> out;
        append_inorder(root_.get(), out);
        return out;
    }

    int height() const { return node_height(root_); }
    bool is_balanced() const { return check_balanced(root_.get()) >= 0; }
    std::size_t size() const { return size_; }
    const Node* inspect_root() const { return root_.get(); }
    static int key(const Node* node) { return node->value; }
    static const Node* left(const Node* node) { return node->left.get(); }
    static const Node* right(const Node* node) { return node->right.get(); }
    static int stored_height(const Node* node) { return node->height + 1; }

private:
    static int node_height(const std::unique_ptr<Node>& node) {
        return node ? node->height : 0;
    }

    static void refresh(Node& node) {
        node.height = std::max(node_height(node.left), node_height(node.right)) + 1;
    }

    static int balance_factor(const std::unique_ptr<Node>& node) {
        return node_height(node->left) - node_height(node->right);
    }

    static std::unique_ptr<Node> rotate_right(std::unique_ptr<Node> node) {
        auto pivot = std::move(node->left);
        node->left = std::move(pivot->right);
        refresh(*node);
        pivot->right = std::move(node);
        refresh(*pivot);
        return pivot;
    }

    static std::unique_ptr<Node> rotate_left(std::unique_ptr<Node> node) {
        auto pivot = std::move(node->right);
        node->right = std::move(pivot->left);
        refresh(*node);
        pivot->left = std::move(node);
        refresh(*pivot);
        return pivot;
    }

    static std::unique_ptr<Node> rebalance(std::unique_ptr<Node> node) {
        refresh(*node);
        if (balance_factor(node) > 1) {
            if (balance_factor(node->left) < 0) {
                node->left = rotate_left(std::move(node->left));
            }
            return rotate_right(std::move(node));
        }
        if (balance_factor(node) < -1) {
            if (balance_factor(node->right) > 0) {
                node->right = rotate_right(std::move(node->right));
            }
            return rotate_left(std::move(node));
        }
        return node;
    }

    static std::unique_ptr<Node> insert_node(std::unique_ptr<Node> node, int value, bool& inserted) {
        if (!node) {
            inserted = true;
            return std::make_unique<Node>(value);
        }
        if (value < node->value) {
            node->left = insert_node(std::move(node->left), value, inserted);
        } else if (node->value < value) {
            node->right = insert_node(std::move(node->right), value, inserted);
        } else {
            return node;
        }
        return rebalance(std::move(node));
    }

    static void append_inorder(const Node* node, std::vector<int>& out) {
        if (!node) {
            return;
        }
        append_inorder(node->left.get(), out);
        out.push_back(node->value);
        append_inorder(node->right.get(), out);
    }

    static int check_balanced(const Node* node) {
        if (!node) {
            return 0;
        }
        int left = check_balanced(node->left.get());
        int right = check_balanced(node->right.get());
        if (left < 0 || right < 0 || std::abs(left - right) > 1 || node->height != std::max(left, right) + 1) {
            return -1;
        }
        return std::max(left, right) + 1;
    }

    std::unique_ptr<Node> root_;
    std::size_t size_{};
};

} // namespace c06_l08
