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
//     在退化树上减少父子层层转发带来的栈增长
//   - 对比普通 `for (int v : inner) co_yield v;` 在退化树上的控制流和栈增长风险
// =====================================================================

#include <coroutine_study/exercise_check.hpp>
#include <exception>

#include <generator>
#include <iostream>
#include <memory>
#include <ranges>
#include <syncstream>
#include <vector>

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

void collect_inorder(Node* root, std::vector<int>& out) {
    if (!root) return;
    collect_inorder(root->left, out);
    out.push_back(root->value);
    collect_inorder(root->right, out);
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
// 进阶 A：构造退化树（深度 200 / 500），画版本 A / B 的恢复链
//   记录栈增长风险即可，默认练习止步于控制流分析。
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶 A]: Node* build_degenerate(int depth);
//   每个节点只有 right（或只有 left），形成一条链；对比两个版本的父子 generator 控制流。

// ─────────────────────────────────────────────────────────────────────
// 进阶 B：在 promise_type 层面给自定义 generator 加 yield_from
// ─────────────────────────────────────────────────────────────────────
// TODO [进阶 B]: 自定义 my_generator + my_promise，实现 await_transform 拦截 inner generator。

}  // namespace

int main() try {
    log("main", "─── B-1：递归 generator 与栈 ───");

    int counter = 1;
    Node* root = build_full(/*depth=*/4, counter);

    log("main", "── 版本 A：elements_of (symmetric transfer)");
    std::vector<int> v1;
    for (int v : inorder_v1_elements_of(root)) {
        v1.push_back(v);
        log("v1", v);
    }

    log("main", "── 版本 B：for + co_yield (普通 resume 链)");
    std::vector<int> v2;
    for (int v : inorder_v2_for_yield(root)) {
        v2.push_back(v);
        log("v2", v);
    }

    std::vector<int> expected;
    collect_inorder(root, expected);
    coroutine_study::check(v1 == expected, "Part 2/3: elements_of path yields the full inorder sequence");
    coroutine_study::check(v2 == expected, "Part 2: recursive for+co_yield path yields the full inorder sequence");

    delete_tree(root);
    return 0;
}
catch (const std::exception& e) {
    std::cerr << "student check failed: " << e.what() << '\n';
    return 1;
}
catch (...) {
    std::cerr << "student check failed: unknown exception\n";
    return 1;
}
