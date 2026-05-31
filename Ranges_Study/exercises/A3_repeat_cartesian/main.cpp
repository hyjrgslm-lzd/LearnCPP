// ============================================================
// 对应章节：../../02-模块A-视图工厂与惰性.md  §练习 A-3
// 小节：repeat_view 与 cartesian_product_view（C++23）
// 提案：P2474R2（std::views::repeat，无界与有界形式）
//        P2374R4（std::views::cartesian_product，多维积视图）
//        P2540R1（cartesian_product_view 空积语义澄清：
//                 零参数调用产出一个 tuple<>，而非空序列）
// C++ 标准要求：C++23
//
// 预期运行输出（骨架阶段）：
//   （无输出；int main() 直接 return 0）
//
// 完成后预期输出：
//   repeat(42) | take(5): 42 42 42 42 42
//   repeat(42, 10) size = 10
//   repeat(42, 10): 42 42 42 42 42 42 42 42 42 42
//   cartesian_product size = 12
//   first element: 1 a true
//   cartesian_product() size = 1
//   got empty tuple
// ============================================================

#include <ranges>
#include <vector>
#include <tuple>
#include <iostream>
#include <print>

int main() {

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1：无界 repeat_view 的 concept 属性
    //   auto inf_42 = std::views::repeat(42);
    //   用 static_assert 验证：
    //     - !std::ranges::sized_range<decltype(inf_42)>      → true
    //     - !std::ranges::common_range<decltype(inf_42)>     → true
    //     -  std::ranges::random_access_range<decltype(inf_42)> → true
    //   用 views::take(5) 限制并打印（42 42 42 42 42）。
    //   警告：千万不要对 inf_42 直接 for-range——这是无限循环！
    // ══════════════════════════════════════════════════════

    auto repeat = std::views::repeat(42);
    for (auto& value : repeat | std::views::take(5))
        std::println("{}", value);

    static_assert(!std::ranges::sized_range<decltype(repeat)>);
    static_assert(!std::ranges::common_range<decltype(repeat)>);
    static_assert(std::ranges::random_access_range<decltype(repeat)>);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2：有界 repeat_view 的 concept 属性
    //   auto bounded_42 = std::views::repeat(42, 10);
    //   用 static_assert 验证：
    //     -  std::ranges::sized_range<decltype(bounded_42)>  → true
    //     -  std::ranges::random_access_range<...>           → true
    //   打印 size（应为 10），再 for-range 打印全部元素。
    // ══════════════════════════════════════════════════════

    auto bounded_repeat = std::views::repeat(42, 10);
    std::println("bounded_repeat size={}", bounded_repeat.size());
    for (auto value : bounded_repeat)
        std::print("{},", value);
    std::println("");

    static_assert(std::ranges::sized_range<decltype(bounded_repeat)>);
    static_assert(std::ranges::random_access_range<decltype(bounded_repeat)>);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3：cartesian_product_view 基本用法
    //   std::vector<int>  nums  = {1, 2, 3};
    //   std::vector<char> chars = {'a', 'b'};
    //   std::vector<bool> flags = {true, false};
    //   auto cp = std::views::cartesian_product(nums, chars, flags);
    //   用 static_assert 验证：
    //     - std::ranges::random_access_range<decltype(cp)>   → true
    //     - std::ranges::sized_range<decltype(cp)>           → true
    //   打印 size（应为 3×2×2 = 12）。
    //   打印第一个元素（用结构化绑定 auto [n,c,f] = *std::ranges::begin(cp)）。
    // ══════════════════════════════════════════════════════

    std::vector<int>  nums  = {1, 2, 3};
    std::vector<char> chars = {'a', 'b'};
    std::vector<bool> flags = {true, false};
    auto cp = std::views::cartesian_product(nums, chars, flags);

    for (auto [n, c, f] : cp)
        std::println("n={}, c={}, f={}", n, c, f);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4：空积语义（P2540R1）
    //   auto empty_product = std::views::cartesian_product();
    //   验证 size 为 1（不是 0！这是数学上空积的定义：{()}）。
    //   for-range 迭代，用 static_assert 验证元素类型是 std::tuple<>。
    //   应打印一行 "got empty tuple"。
    //   思考：空积为何是 1 而不是 0？与 empty_view 的 0 个元素有何不同？
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1：repeat_view 的元素引用语义
    //   验证无界 repeat(42) 所有迭代器解引用指向同一个对象：
    //     auto r = std::views::repeat(42);
    //     auto it1 = r.begin();
    //     auto it2 = std::next(it1);
    //     assert(&*it1 == &*it2);   // 同一个 const int 对象
    //   思考：这说明 view 只存储一个值，迭代器每次解引用都返回该值的引用。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2：cartesian_product + filter 的 iterator_concept 降级
    //   auto sides = std::views::iota(1, 21);
    //   auto triples = std::views::cartesian_product(sides, sides, sides)
    //       | std::views::filter([](auto t) {
    //           auto [a, b, c] = t;
    //           return a <= b && b <= c && a*a + b*b == c*c;
    //       });
    //   验证 triples 至少满足 bidirectional_range（filter_view 给出 bidirectional 上界）。
    //   打印所有勾股数三元组（3 4 5, 5 12 13, ...）。
    //   注意：含 filter 的管道不能存为 const（begin() 是非 const 成员）。
    // ══════════════════════════════════════════════════════

    return 0;
}

// ---- static_assert 验证区 ----
// 全局类型级验证（不依赖局部变量）：

// // [必做 1] 无界 repeat_view
// static_assert(!std::ranges::sized_range<
//     std::ranges::repeat_view<int>>);
// static_assert(!std::ranges::common_range<
//     std::ranges::repeat_view<int>>);
// static_assert( std::ranges::random_access_range<
//     std::ranges::repeat_view<int>>);

// // [必做 2] 有界 repeat_view
// static_assert( std::ranges::sized_range<
//     std::ranges::repeat_view<int, std::ptrdiff_t>>);

// // [必做 3] cartesian_product：三个 random_access 维度 → random_access
// static_assert( std::ranges::random_access_range<
//     std::ranges::cartesian_product_view<
//         std::ranges::iota_view<int,int>,
//         std::ranges::iota_view<int,int>>>);
