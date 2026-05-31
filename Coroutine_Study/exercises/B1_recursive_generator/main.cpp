// =====================================================================
// 练习 B-1：递归 generator 与栈
//   对应文档：03-模块B-generator与task的使用.md / 练习 B-1
//   官方参考：
//     - P2502R2:     https://wg21.link/P2502R2
//     - cppreference: https://en.cppreference.com/w/cpp/coroutine/generator
//     - Lewis Baker (symmetric transfer):
//                   https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
//
// 学习目标：
//   - 看见 P2502R2 `co_yield std::ranges::elements_of(...)` 触发的 symmetric transfer
//     在退化树上不爆栈
//   - 看见普通 `for (int v : inner) co_yield v;` 在退化树上和普通递归一样爆栈
// =====================================================================

#include <generator>
#include <iostream>
#include <memory>
#include <ranges>
#include <syncstream>

namespace {

template <class... Args>
void log(const char* tag, Args&&... args) {
    std::osyncstream os{std::cout};
    os << "[" << tag << "] ";
    ((os << args), ...);
    os << "\n";
}

struct Node {
    int   value;
    Node* left{};
    Node* right{};
};

// 简易构建：构造一棵满二叉树，深度 depth，按层序赋值
Node* build_full(int depth, int& counter) {
    if (depth <= 0) return nullptr;
    auto* n = new Node{counter++, nullptr, nullptr};
    n->left  = build_full(depth - 1, counter);
    n->right = build_full(depth - 1, counter);
    return n;
}

void delete_tree(Node* n) {
    if (!n) return;
    delete_tree(n->left);
    delete_tree(n->right);
    delete n;
}

// ─────────────────────────────────────────────────────────────────────
// 版本 A（推荐）：P2502 symmetric transfer
//   left/right 子树用 co_yield std::ranges::elements_of(...)
// ─────────────────────────────────────────────────────────────────────
std::generator<int> inorder_v1_elements_of(Node* root) {
    if (!root) co_return;
    // TODO [必做 A]:
    //   co_yield std::ranges::elements_of(inorder_v1_elements_of(root->left));
    //   co_yield root->value;
    //   co_yield std::ranges::elements_of(inorder_v1_elements_of(root->right));
    //
    // 占位实现：仅 yield 当前节点值，让骨架默认可编译。
    co_yield root->value;
}

// ─────────────────────────────────────────────────────────────────────
// 版本 B（反例）：普通 for + co_yield，不触发 symmetric transfer
// ─────────────────────────────────────────────────────────────────────
std::generator<int> inorder_v2_for_yield(Node* root) {
    if (!root) co_return;
    // TODO [必做 B]:
    //   for (int v : inorder_v2_for_yield(root->left))  co_yield v;
    //   co_yield root->value;
    //   for (int v : inorder_v2_for_yield(root->right)) co_yield v;
    //
    // 占位实现：同上。
    co_yield root->value;
}

// ─────────────────────────────────────────────────────────────────────
// 进阶 A：构造退化树（深度 200 / 500），分别用版本 A / B 遍历
//   预期：A 安然无恙，B 在某个深度爆栈
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶 A]: Node* build_degenerate(int depth);
//   每个节点只有 right（或只有 left），形成一条链；用版本 A 跑通，再用版本 B 触发栈溢出。

// ─────────────────────────────────────────────────────────────────────
// 进阶 B：在 promise_type 层面给自定义 generator 加 yield_from
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶 B]: 自定义 my_generator + my_promise，实现 await_transform 拦截 inner generator。

}  // namespace

int main() {
    log("main", "─── B-1：递归 generator 与栈 ───");

    int counter = 1;
    Node* root = build_full(/*depth=*/4, counter);

    log("main", "── 版本 A：elements_of (symmetric transfer)");
    for (int v : inorder_v1_elements_of(root)) {
        log("v1", v);
    }

    log("main", "── 版本 B：for + co_yield (普通 resume 链)");
    for (int v : inorder_v2_for_yield(root)) {
        log("v2", v);
    }

    delete_tree(root);
    return 0;
}
