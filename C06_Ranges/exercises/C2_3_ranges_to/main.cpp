// 模块 C2 · 练习 C2-3：ranges::to 与 from_range_t
// 提案：P1206R7（ranges::to，四阶回退路径、CTAD、管道语法）
//        P2781R5（std::from_range_t，容器构造侧 range 初始化标签）
// 标准：C++26（ranges::to 和 from_range_t 均为 C++23）
//
// 预期输出（空白 main，仅 static_assert）：
//   （无输出，静默通过）
//
// 完成必做 TODO 后预期输出：
//   odd 1-10: 1 3 5 7 9
//   CTAD vector: 1 2 3 4 5
//   string: abcde
//   from_range == ranges::to: true
//   MinimalContainer: 1 2 3 4 5

#include <ranges>
#include <vector>
#include <list>
#include <string>
#include <iostream>

// TODO [必做] 3（最小容器定义区）：
// 在此处定义 MinimalContainer，只提供 push_back(int) + begin()/end()。
// 不实现 from_range_t 构造或 insert_range，验证 ranges::to 走第 4 条回退路径。

int main()
{
    // TODO [必做] 1: ranges::to 三种语法
    //   a) 管道语法：iota(1,11) | filter(odd) | ranges::to<vector<int>>()
    //      打印 1 3 5 7 9。
    //   b) 函数调用：ranges::to<vector<int>>(iota(1,6))
    //   c) CTAD：iota(1,6) | ranges::to<vector>()
    //      用 static_assert 验证推导结果是 vector<int>。

    // TODO [必做] 2: 转成不同容器
    //   iota(1,6) | ranges::to<list<int>>()
    //   iota('a','f') | ranges::to<string>()  → 打印 abcde

    // TODO [必做] 3: MinimalContainer — 验证第 4 条回退路径
    //   实现 MinimalContainer（push_back 版），
    //   在 push_back 内加 cout 输出，确认 ranges::to 逐一调用了 push_back。
    //   打印最终容器内容 1 2 3 4 5。

    // TODO [必做] 4: from_range_t 标签协议
    //   vector<int> v(std::from_range, views::iota(1,6))
    //   用 auto vb = iota(1,6) | ranges::to<vector<int>>()
    //   验证 v == vb 为 true，说明两条路径等价。
    //   static_assert(is_same_v<decltype(std::from_range), const std::from_range_t>)

    // TODO [必做] 5: "忘括号"的错误演示
    //   在注释里写出 r | ranges::to<vector<int>>（无括号）并说明编译错误原因。

    // TODO [进阶] 1: ranges::to 四阶选择顺序
    //   在注释里列出四阶优先级，并指出 std::vector 走哪条路径、
    //   MinimalContainer 走哪条路径。

    // TODO [进阶] 2: 嵌套容器 map<string, vector<int>>
    //   构造 vector<pair<string, vector<int>>> raw，
    //   用 ranges::to<map<string,vector<int>>>() 直接转换并打印。

    return 0;
}

// ── static_assert 验证区 ──────────────────────────────────────────────────
// 完成必做 1、4 后，把下列 assert 从注释中解开并确认编译通过。

// static_assert(std::same_as<
//     decltype(std::views::iota(1,6) | std::ranges::to<std::vector>()),
//     std::vector<int>>);
//
// static_assert(std::is_same_v<
//     decltype(std::from_range),
//     const std::from_range_t>);
