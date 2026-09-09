// 模块 C1 · 练习 C1-1：join 与嵌套 range 的拍平
// 提案：P0896R4（C++20 join_view）、P2441R2（C++23 join_with）
// 标准：C++26（主体 C++20，join_with 需 C++23）
//
// 预期输出（空白 main，仅 static_assert）：
//   （无输出，静默通过）
//
// 完成必做 TODO 后预期输出：
//   nested join: 1 2 3 4 5 6
//   iota join:   0 0 1 0 1 2

#include <ranges>
#include <vector>
#include <iostream>
#include <print>

int main()
{
    // TODO [必做] 1: 构造 vector<vector<int>> = {{1,2,3},{4,5},{6}}
    //   用 views::join 拍平，for-range 循环打印，确认输出 1 2 3 4 5 6。
    //   在注释中说明：join_view 的 begin() 与 end() 类型是否相同？

    auto v = std::vector<std::vector<int>>{ {1,2,3},{4,5},{6} };

    for (auto value : std::views::join(v))
        std::print("{} ", value);
    std::println("");

    // TODO [必做] 2: 用 views::iota(1,4) | views::transform(n->iota(0,n)) | views::join
    //   打印 0 0 1 0 1 2，体会"transform 生成右值 view 作为内层"的情形。

    auto t2 = std::views::iota(1, 4) | std::views::transform([](int n) { return std::views::iota(0, n); }) | std::views::join;
    for (auto value : t2)
        std::print("{} ", value);
    std::println("");

    // TODO [必做] 3: 声明 flat = nested | views::join，用 static_assert 验证：
    //   - bidirectional_iterator<It>  为 true
    //   - random_access_iterator<It>  为 false
    //   - common_range<decltype(flat)> 为 false（begin/end 类型不同）

    // TODO [必做] 4: 在注释里回答：
    //   外层 random_access_range + 内层 random_access_range，
    //   为什么 join_view 的迭代器仍只能达到 bidirectional？

    // TODO [进阶] 1: 在 C++23 下用 views::join_with('-') 把
    //   vector<string>{"hello","world","ranges"} 输出为 hello-world-ranges。
    //   对比 views::join（无分隔符）。

    auto t3 = v | std::views::join_with(-1);
    for (auto value : t3)
        std::print("{} ", value);
    std::println("");

    // TODO [进阶] 2: 解释 join_with 的分隔符插入时机：
    //   是"每个内层范围之前"还是"相邻内层范围之间"？第一个内层前有无分隔符？

    return 0;
}

// ── static_assert 验证区 ──────────────────────────────────────────────────
// 完成必做 3 后，把下列 assert 从注释中解开并确认编译通过。

// static_assert([](){
//     std::vector<std::vector<int>> nested = {{1,2},{3,4}};
//     auto flat = nested | std::views::join;
//     using It = decltype(flat.begin());
//     static_assert(std::bidirectional_iterator<It>);
//     static_assert(!std::random_access_iterator<It>);
//     static_assert(!std::ranges::common_range<decltype(flat)>);
//     return true;
// }());
