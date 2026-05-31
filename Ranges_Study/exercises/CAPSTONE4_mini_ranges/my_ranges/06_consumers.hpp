#pragma once
// =============================================================================
// my_ranges/06_consumers.hpp — 层 6：消费端骨架（ranges::to）
//
// 对应章节：模块 G/H — ranges::to，管道终端消费
//
// 设计说明：
//   ranges::to<C>() 返回一个 closure 对象，继承 range_adaptor_closure，
//   可以出现在管道右侧：iota | transform | take | to<vector<int>>()
//
//   本实现为简化版：
//   - 假设 C 有 push_back 成员（不覆盖 set / map 等）
//   - 不做 reserve 优化（进阶：对 sized_range 调用 c.reserve(ranges::size(r))）
//   - 元素类型兼容性通过 push_back 的构造函数隐式转换保证
//
// TODO 列表：
//   [必做] 6a. 补全 _to_closure::operator() 的元素插入循环（当前已给出骨架）
//   [必做] 6b. 验证管道 iota(1,11) | transform(x*x) | take(5) | to<vector<int>>() 编译运行
//   [进阶] 6c. 对 sized_range 增加 reserve 优化
//   [进阶] 6d. 增加支持 insert-based 容器（set, map）的重载路径
//   [进阶] 6e. 支持嵌套范围（ranges::to<vector<vector<int>>>，递归 to）
// =============================================================================

#include "05_adaptors.hpp"
#include <utility>

namespace my::ranges {

// ---------------------------------------------------------------------------
// _to_closure<C>：ranges::to<C>() 返回此类型
// ---------------------------------------------------------------------------
template<typename C>
struct _to_closure
    : my::ranges::range_adaptor_closure<_to_closure<C>>
{
    // TODO [必做] 6a：对 input_range R，将所有元素插入到 C 并返回
    template<my::ranges::input_range R>
    C operator()(R&& r) const {
        C result;
        // TODO [进阶] 6c：若 R 是 sized_range，先 result.reserve(my::ranges::size(r))
        for (auto&& e : r) {
            // TODO [必做] 6a：使用 push_back 插入（forward 保留值类别）
            result.push_back(std::forward<decltype(e)>(e));
        }
        return result;
    }
};

// ---------------------------------------------------------------------------
// to<C>()：工厂函数，返回 _to_closure<C>
// ---------------------------------------------------------------------------
// TODO [必做] 6b：验证以下调用链编译运行，结果为 {1, 4, 9, 16, 25}：
//   auto v = my::views::iota(1, 11)
//          | my::views::transform([](int x){ return x * x; })
//          | my::views::take(5)
//          | my::ranges::to<std::vector<int>>();
//   assert((v == std::vector<int>{1, 4, 9, 16, 25}));
template<typename C>
constexpr auto to() {
    return _to_closure<C>{};
}

} // namespace my::ranges
