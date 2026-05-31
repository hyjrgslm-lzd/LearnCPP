// ============================================================
// 对应章节：../../02-模块A-视图工厂与惰性.md  §练习 A-1
// 小节：iota 与无界范围
// 提案：P0896R4（C++20 ranges 基础，含 iota_view 初始设计）
// C++ 标准要求：C++20
//
// 预期运行输出（骨架阶段）：
//   （无输出；int main() 直接 return 0）
//
// 完成后预期输出：
//   bounded size = 10
//   0 1 2 3 4
//   20 30 40
// ============================================================

#include <ranges>
#include <iterator>
#include <vector>
#include <iostream>
#include <print>

int main() {

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1：有界 iota_view 的 concept 属性
    //   构造 auto bounded = std::views::iota(0, 10);
    //   用 static_assert 验证：
    //     - std::ranges::sized_range<decltype(bounded)>      → true
    //     - std::ranges::common_range<decltype(bounded)>     → true
    //     - std::ranges::random_access_range<decltype(bounded)> → true
    //   打印 std::ranges::size(bounded)，应为 10。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2：无界 iota_view 的 concept 属性
    //   构造 auto unbounded = std::views::iota(0);
    //   用 static_assert 验证：
    //     - !std::ranges::sized_range<decltype(unbounded)>   → true（无法 O(1) 求大小）
    //     - !std::ranges::common_range<decltype(unbounded)>  → true（end 类型不同）
    //     -  std::ranges::random_access_range<...>           → true（iterator 支持算术）
    //   用 std::same_as<> 验证 end() 类型是 std::unreachable_sentinel_t。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3：iterator_concept 检查
    //   对 unbounded，取 std::ranges::iterator_t<decltype(unbounded)>，
    //   验证其 ::iterator_concept 是 std::random_access_iterator_tag。
    //   提示：using It = std::ranges::iterator_t<decltype(unbounded)>;
    //         static_assert(std::same_as<typename It::iterator_concept, ...>);
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4：用 take 把无界变有界
    //   auto first5 = unbounded | std::views::take(5);
    //   验证 first5 满足 sized_range，并用 for-range 打印前 5 个元素（0 1 2 3 4）。
    //   思考：take 返回的是 view（蓝图），不是容器，迭代从 for-range 才开始。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 5：subrange 包装已有迭代器对
    //   std::vector<int> v = {10, 20, 30, 40, 50};
    //   用 v.begin()+1 和 v.begin()+4 构造 std::ranges::subrange。
    //   验证其为 sized_range + common_range，for-range 打印 20 30 40。
    //   思考：subrange 是对已有 iterator pair 的轻量包装，不生成新值。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1：自定义终止条件 sentinel
    //   用 views::take_while 模拟"遇到某条件停止"的有界序列：
    //     auto evens_under_20 = std::views::iota(0)
    //         | std::views::filter([](int x){ return x % 2 == 0; })
    //         | std::views::take_while([](int x){ return x < 20; });
    //   用 static_assert 验证 evens_under_20 不满足 common_range
    //   （take_while 的 end 类型与 begin 类型不同）。
    //   打印结果（0 2 4 6 8 10 12 14 16 18）。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2：take(n) 的 sized_range 传播
    //   auto first10 = std::views::iota(0) | std::views::take(10);
    //   验证 first10 满足 sized_range，打印其 size（应为 10）。
    //   对比：有界 iota | take vs 无界 iota | take，
    //         两者产出的 view 的 sized_range 属性是否相同？为什么？
    // ══════════════════════════════════════════════════════

    // // [必做 1] 有界 iota
    static_assert( std::ranges::sized_range<std::ranges::iota_view<int,int>>);
    static_assert( std::ranges::common_range<std::ranges::iota_view<int,int>>);
    static_assert( std::ranges::random_access_range<std::ranges::iota_view<int,int>>);

    // // [必做 2] 无界 iota
    static_assert(!std::ranges::sized_range<
                  std::ranges::iota_view<int, std::unreachable_sentinel_t>>);
    static_assert(!std::ranges::common_range<
                  std::ranges::iota_view<int, std::unreachable_sentinel_t>>);
    static_assert( std::ranges::random_access_range<
                  std::ranges::iota_view<int, std::unreachable_sentinel_t>>);

    // // [必做 3] iterator_concept
    static_assert(std::same_as<
                  typename std::ranges::iterator_t<
                  std::ranges::iota_view<int,std::unreachable_sentinel_t>
                  >::iterator_concept,
                  std::random_access_iterator_tag>);




    return 0;
}

// ---- static_assert 验证区 ----
// 完成 TODO 后逐一取消注释：

