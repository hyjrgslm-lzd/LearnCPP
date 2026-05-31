// ============================================================
// 对应章节：../../02-模块A-视图工厂与惰性.md  §练习 A-2
// 小节：istream_view 与单遍迭代
// 提案：P0896R4（C++20 ranges 基础，含 istream_view 初始设计）
//        P1207R4（Movability of single-pass iterators；
//                 input_iterator 不再要求 copyable）
//        P1035R7（C++20 ranges 对标准算法的概念化改造）
// C++ 标准要求：C++20（进阶 ranges::to 需要 C++23）
//
// 预期运行输出（骨架阶段）：
//   （无输出；int main() 直接 return 0）
//
// 完成后预期输出：
//   first pass: 1 2 3 4 5
//   second pass (same stream): (empty)
//   copied: 10 20 30 40 50
// ============================================================

#include <ranges>
#include <iterator>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <print>

int main() {

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1：istream_view 的 concept 属性验证
    //   std::istringstream iss1{"1 2 3 4 5"};
    //   auto iv1 = std::views::istream<int>(iss1);
    //   用 static_assert 验证：
    //     -  std::ranges::input_range<decltype(iv1)>    → true
    //     - !std::ranges::forward_range<decltype(iv1)>  → true（单遍，无 multi-pass 保证）
    //     - !std::ranges::sized_range<decltype(iv1)>    → true（不知道流有多少元素）
    //     - !std::ranges::common_range<decltype(iv1)>   → true（end 是 sentinel）
    // ══════════════════════════════════════════════════════

    std::istringstream iss1{ "1 2 3 4 5" };
    auto iv1 = std::views::istream<int>(iss1);

    static_assert(std::ranges::input_range<decltype(iv1)>);
    static_assert(!std::ranges::forward_range<decltype(iv1)>);
    static_assert(!std::ranges::sized_range<decltype(iv1)>);
    static_assert(!std::ranges::common_range<decltype(iv1)>);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2：单遍性演示
    //   第一次 for-range 消费 iv1，打印 "first pass: 1 2 3 4 5"。
    //   第二次 for-range 对同一个 iv1 迭代，打印
    //     "second pass (same stream): (empty)"（什么都不产出）。
    //   思考：为什么第二次为空？（流已到 EOF，begin() 不幂等）
    // ══════════════════════════════════════════════════════

    std::println("before tellg: {}", static_cast<std::streamoff>(iss1.tellg()));

    for (auto iter : iv1)
        std::println("{}", iter);

    std::println("first pass tellg: {}", static_cast<std::streamoff>(iss1.tellg()));

    for (auto iter : iv1)
        std::println("{}", iter);

    std::println("end tellg: {}", static_cast<std::streamoff>(iss1.tellg()));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3：iterator_concept 与 move-only iterator（P1207R4）
    //   取 iterator 类型：
    //     using It = std::ranges::iterator_t<decltype(iv1)>;
    //   验证：
    //     - iterator_concept 是 std::input_iterator_tag
    //     - iterator 不满足 std::copyable（move-only，P1207R4 放宽的结果）
    //   提示：static_assert(!std::copyable<It>);
    // ══════════════════════════════════════════════════════

    using iter = std::ranges::iterator_t<decltype(iv1)>;
    static_assert(std::same_as<typename iter::iterator_concept, std::input_iterator_tag>);
    static_assert(!std::copyable<iter>);

    auto vec = std::ranges::to<std::vector<int>>(iv1);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4：用 ranges::copy 把 istream_view 写入 vector
    //   std::istringstream iss2{"10 20 30 40 50"};
    //   auto iv2 = std::views::istream<int>(iss2);
    //   std::vector<int> result;
    //   std::ranges::copy(iv2, std::back_inserter(result));
    //   打印 result 的内容（10 20 30 40 50）。
    //   思考：copy 是算法，把元素复制到已有目标；to<> 是容器化操作（见进阶）。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1：用 ranges::to<vector>() 替代 copy + back_inserter（C++23）
    //   std::istringstream iss3{"100 200 300"};
    //   auto iv3 = std::views::istream<int>(iss3);
    //   auto vec = std::ranges::to<std::vector<int>>(iv3);
    //   打印 vec。对比两种写法（copy vs to）的语义差异：
    //     - copy：算法，需要预先准备 back_inserter
    //     - to：容器化操作，直接产出容器，语义更完整
    //   注意：两种方式对 input_range 都只能单遍消费。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2：begin() 不幂等的直接证明
    //   在第一次 for-range 之前打印 iss1.tellg()，
    //   for-range 结束后再打印，观察位置已前进到末尾（-1 表示 EOF）。
    //   提示：istringstream::tellg() 返回当前读取位置。
    // ══════════════════════════════════════════════════════

    return 0;
}

// ---- static_assert 验证区 ----
// 以下断言依赖具体的 iv1 类型，在局部作用域外无法直接使用；
// 将它们移入 main() 中填写 TODO 时取消注释验证。

// 类型级验证（可在全局作用域用具体特化类型验证）：
// using IstreamViewInt = std::ranges::istream_view<int>;
// static_assert( std::ranges::input_range<IstreamViewInt>);
// static_assert(!std::ranges::forward_range<IstreamViewInt>);
// static_assert(!std::ranges::sized_range<IstreamViewInt>);
// static_assert(!std::ranges::common_range<IstreamViewInt>);
// static_assert(!std::copyable<std::ranges::iterator_t<IstreamViewInt>>);
// static_assert( std::same_as<
//     typename std::ranges::iterator_t<IstreamViewInt>::iterator_concept,
//     std::input_iterator_tag>);
