// 模块 D — 练习 D-3：std::generator 协程桥
// 章节：06-模块D-C++23高阶视图与协程桥.md §练习 D-3
// 提案：P2502R2 (std::generator, elements_of)
// C++ 标准：C++23
//
// 预期输出（填完 TODO 后）：
//   fib() | take(10): 0 1 1 2 3 5 8 13 21 34
//   fib | take(10) | to<vector>: 0 1 1 2 3 5 8 13 21 34
//   first 5 via manual iterator: 0 1 1 2 3
//   next 5 via manual iterator: 5 8 13 21 34
//   DFS walk_tree: 1 2 4 5 3
//   nodes >= 3: 3 4 5
//   flatten via elements_of: 1 2 3 4 5 6
//
// 关键概念：
//   - std::generator<T>：input_range + view，move-only，单遍（不满足 forward_range）
//   - 迭代器是 move-only input_iterator：无 multi-pass 保证
//   - co_yield std::ranges::elements_of(range)：委托另一 range 产值（symmetric transfer）
//   - generator 协程里只能用 co_yield，不能混 co_await
//   - initial_suspend 返回 suspend_always：惰性启动（被消费时才开始执行）

#include <generator>
#include <ranges>
#include <vector>
#include <memory>
#include <iostream>
#include <string>

// ============================================================
// TODO [必做] 1: 实现斐波那契无限生成器
//   std::generator<int> fib() {
//       int a = 0, b = 1;
//       while (true) { co_yield a; auto next = a+b; a=b; b=next; }
//   }
//   配合 views::take(10) 截取前 10 个值
//   用 ranges::to<vector<int>>() 一次性收集
// ============================================================

// ============================================================
// TODO [必做] 2: 用 static_assert 验证 generator 的 concept
//   static_assert(std::ranges::input_range<std::generator<int>>);
//   static_assert(std::ranges::view<std::generator<int>>);
//   static_assert(!std::ranges::forward_range<std::generator<int>>);
//   说出原因：迭代器 move-only，无 multi-pass 保证
// ============================================================

// ============================================================
// TODO [必做] 3: 验证 generator 是 move-only view
//   auto g1 = fib();
//   auto g2 = std::move(g1);   // OK
//   // auto g3 = g2;            // 编译错误
//   static_assert(!std::copyable<std::generator<int>>);
//   static_assert(std::movable<std::generator<int>>);
// ============================================================

// ============================================================
// TODO [必做] 4: 演示 generator 的单遍语义
//   auto g = fib();
//   手动用 it/end 推进：先取 5 个，再取接续的 5 个
//   证明 generator 不能从头重播（第二次从第一次末尾继续）
// ============================================================

// ============================================================
// TODO [必做] 5: 实现树的前序递归遍历（elements_of 版）
//   struct Node { int value; std::vector<Node> children; };
//   std::generator<int> walk_tree(const Node& node) {
//       co_yield node.value;
//       for (const auto& child : node.children)
//           co_yield std::ranges::elements_of(walk_tree(child));
//   }
//   构造树 1→{2→{4,5}, 3}，DFS 应输出 1 2 4 5 3
// ============================================================

// ============================================================
// TODO [必做] 6: 把 generator 接入 ranges 管道
//   walk_tree(tree) | views::filter([](int v){ return v >= 3; })
//   应产出 3 4 5
// ============================================================

// ============================================================
// TODO [进阶] 1: 实现 flatten 演示 elements_of 委托语义
//   std::generator<int> flatten(std::vector<std::vector<int>> data) {
//       for (auto& row : data)
//           co_yield std::ranges::elements_of(row);
//   }
//   flatten({{1,2,3},{4,5},{6}}) | ranges::to<vector>()
//   产出 1 2 3 4 5 6
// ============================================================

// TODO [进阶] 2: 对比 views::repeat + transform（有限表达）与 generator（有状态机）
//   views::repeat(42) | take(5)：固定值，无状态
//   fib() | take(5)：每步更新 a,b，有状态转移
//   说明 generator 的优势：任意复杂的协程逻辑（条件、循环、递归）均可 co_yield
// ============================================================

// TODO [进阶] 3: 把 walk_tree 改为后序遍历（先子节点后父节点）
//   对比前序与后序的 elements_of 写法差异
// ============================================================

// TODO [进阶] 4: 验证 move 后继续使用原 generator 的行为
//   auto g2 = std::move(g1); 后 g1 处于移后状态
//   继续迭代 g1 是未定义行为（不要在生产代码里这样做）
// ============================================================

int main() {
    std::cout << "D3 skeleton — fill TODOs to explore std::generator\n";
    return 0;
}

// ============================================================
// 静态验证区（填完 TODO 后解注释）
// ============================================================
// static_assert(std::ranges::input_range<std::generator<int>>);
// static_assert(std::ranges::view<std::generator<int>>);
// static_assert(!std::ranges::forward_range<std::generator<int>>);
// static_assert(!std::copyable<std::generator<int>>);
// static_assert(std::movable<std::generator<int>>);
