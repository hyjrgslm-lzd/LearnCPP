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
        int stored = 1;
        std::unique_ptr<Node> smaller;
        std::unique_ptr<Node> larger;
    };

    bool insert(int value) {
        bool grew = false;
        root_ = add(std::move(root_), value, grew);
        size_ += grew ? 1u : 0u;
        return grew;
    }

    bool contains(int value) const {
        const Node* current = root_.get();
        while (current != nullptr) {
            if (value < current->value) {
                current = current->smaller.get();
            } else if (current->value < value) {
                current = current->larger.get();
            } else {
                return true;
            }
        }
        return false;
    }

    std::vector<int> inorder() const {
        std::vector<int> values;
        visit(root_.get(), values);
        return values;
    }

    int height() const { return height(root_.get()); }
    bool is_balanced() const { return verify(root_.get()) >= 0; }
    std::size_t size() const { return size_; }

    const Node* inspect_root() const { return root_.get(); }
    static int key(const Node* node) { return node->value; }
    static const Node* left(const Node* node) { return node->smaller.get(); }
    static const Node* right(const Node* node) { return node->larger.get(); }
    static int stored_height(const Node* node) { return node->stored; }

private:
    static int height(const Node* node) {
        return node == nullptr ? 0 : node->stored;
    }

    static void update(Node* node) {
        node->stored = std::max(height(node->smaller.get()), height(node->larger.get())) + 1;
    }

    static std::unique_ptr<Node> promote_left(std::unique_ptr<Node> old_root) {
        auto new_root = std::move(old_root->larger);
        old_root->larger = std::move(new_root->smaller);
        update(old_root.get());
        new_root->smaller = std::move(old_root);
        update(new_root.get());
        return new_root;
    }

    static std::unique_ptr<Node> promote_right(std::unique_ptr<Node> old_root) {
        auto new_root = std::move(old_root->smaller);
        old_root->smaller = std::move(new_root->larger);
        update(old_root.get());
        new_root->larger = std::move(old_root);
        update(new_root.get());
        return new_root;
    }

    static std::unique_ptr<Node> fix(std::unique_ptr<Node> node) {
        update(node.get());
        int balance = height(node->smaller.get()) - height(node->larger.get());
        if (balance == 2) {
            if (height(node->smaller->smaller.get()) < height(node->smaller->larger.get())) {
                node->smaller = promote_left(std::move(node->smaller));
            }
            return promote_right(std::move(node));
        }
        if (balance == -2) {
            if (height(node->larger->larger.get()) < height(node->larger->smaller.get())) {
                node->larger = promote_right(std::move(node->larger));
            }
            return promote_left(std::move(node));
        }
        return node;
    }

    static std::unique_ptr<Node> add(std::unique_ptr<Node> node, int value, bool& inserted) {
        if (node == nullptr) {
            inserted = true;
            return std::make_unique<Node>(value);
        }
        if (value < node->value) {
            node->smaller = add(std::move(node->smaller), value, inserted);
        } else if (node->value < value) {
            node->larger = add(std::move(node->larger), value, inserted);
        } else {
            return node;
        }
        return fix(std::move(node));
    }

    static void visit(const Node* node, std::vector<int>& out) {
        if (node == nullptr) {
            return;
        }
        visit(node->smaller.get(), out);
        out.push_back(node->value);
        visit(node->larger.get(), out);
    }

    static int verify(const Node* node) {
        if (node == nullptr) {
            return 0;
        }
        int left_h = verify(node->smaller.get());
        int right_h = verify(node->larger.get());
        if (left_h < 0 || right_h < 0 || std::abs(left_h - right_h) > 1 || node->stored != std::max(left_h, right_h) + 1) {
            return -1;
        }
        return std::max(left_h, right_h) + 1;
    }

    std::unique_ptr<Node> root_;
    std::size_t size_{};
};

} // namespace c06_l08
