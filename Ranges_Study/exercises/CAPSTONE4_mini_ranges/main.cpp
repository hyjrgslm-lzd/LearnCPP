// =============================================================================
// CAPSTONE4_mini_ranges — main.cpp
//
// 项目目标：从零实现最小但可用的 mini-ranges 子系统。
// 综合考察阶段二（模块 E-H）全部实现技术。
//
// 六层架构（从底层到上层）：
//
//   层 6 — 消费层   (my_ranges/06_consumers.hpp)  ranges::to<C>
//   层 5 — closure层 (my_ranges/05_adaptors.hpp)   transform/take closure
//   层 4 — view 层  (my_ranges/04_factories.hpp)  iota_view / single_view
//                   (my_ranges/05_adaptors.hpp)   transform_view / take_view
//   层 3 — 基础设施层(my_ranges/03_interface.hpp) view_interface / range_adaptor_closure
//   层 2 — 概念层   (my_ranges/02_concepts.hpp)   range / view / input_range / forward_range
//   层 1 — CPO 层   (my_ranges/01_cpo.hpp)        begin / end / iter_move / size
//
// 最终验收样例：
//   auto v = my::views::iota(1, 11)
//          | my::views::transform([](int x){ return x * x; })
//          | my::views::take(5)
//          | my::ranges::to<std::vector<int>>();
//   // v == {1, 4, 9, 16, 25}
//   assert((v == std::vector<int>{1, 4, 9, 16, 25}));
//
// 使用方式：
//   从顶层到底层，逐层填写各 .hpp 文件的 TODO，然后在此文件取消注释对应验证块。
//   每完成一层，运行编译验证该层的 static_assert 全部通过，再进入下一层。
// =============================================================================

#include "my_ranges/06_consumers.hpp"  // 包含全部六层
#include <vector>
#include <cassert>
#include <cstdio>

// =============================================================================
// TODO [必做] 1：层 1 验证 — CPO 是函数对象，不是函数模板
// =============================================================================
// 层 1 完成后取消注释：

/*
// 1a. begin / end 可以作为值存储（函数模板指针无法这样做）
static_assert(!std::is_function_v<decltype(my::ranges::begin)>,
    "begin 应是函数对象，不是函数");

// 1b. iter_move 对普通迭代器等价于 std::move(*it)
static_assert([] {
    int arr[] = {1, 2, 3};
    auto it = arr;
    return my::ranges::iter_move(it) == 1;
}());
*/

// =============================================================================
// TODO [必做] 2：层 2 验证 — concept 约束
// =============================================================================
// 层 2 完成后取消注释：

/*
// 2a. vector<int> 满足 range（有 begin/end 成员）
static_assert(my::ranges::range<std::vector<int>>,
    "vector<int> 应满足 my::ranges::range");

// 2b. vector<int> 不满足 view（未设 enable_view）
static_assert(!my::ranges::view<std::vector<int>>,
    "vector<int> 不应满足 my::ranges::view");

// 2c. vector<int> 满足 input_range / forward_range
static_assert(my::ranges::input_range<std::vector<int>>);
static_assert(my::ranges::forward_range<std::vector<int>>);
*/

// =============================================================================
// TODO [必做] 3：层 3 + 4 验证 — iota_view 是 view；enable_borrowed_range
// =============================================================================
// 层 4 完成后取消注释：

/*
// 3a. iota_view<int> 满足 view
static_assert(my::ranges::view<my::views::iota_view<int>>,
    "iota_view 应满足 my::ranges::view");

// 3b. iota_view<int> 的 enable_borrowed_range = true
static_assert(my::ranges::enable_borrowed_range<my::views::iota_view<int>>,
    "iota_view 应标记为 borrowed_range");

// 3c. single_view<int> 是 view；begin() 返回 int*（contiguous）
static_assert(my::ranges::view<my::views::single_view<int>>);
static_assert(std::is_same_v<
    decltype(my::views::single_view<int>{42}.begin()),
    int*
>);

// 3d. view_interface 注入的 empty() / front() 可调用
static_assert([] {
    auto sv = my::views::single_view<int>{42};
    return !sv.empty() && sv.front() == 42;
}());
*/

// =============================================================================
// TODO [必做] 4：层 5 验证 — take_view sentinel 异型
// =============================================================================
// 层 5 完成后取消注释：

/*
// 4a. take_view 的 begin() 和 end() 返回类型不同（sentinel 异型）
static_assert([] {
    auto tv = my::views::take_view{my::views::iota_view{0, 100}, 5};
    using begin_t = decltype(tv.begin());
    using end_t   = decltype(tv.end());
    return !std::is_same_v<begin_t, end_t>;
}(), "take_view 的 begin/end 应为异型（sentinel 设计）");

// 4b. transform_view::iterator::iterator_concept 是 random_access（iota 底层 random_access）
static_assert([] {
    using tv_t = decltype(
        my::views::iota_view{0, 10} | my::views::transform([](int x){ return x*x; })
    );
    using iter = my::ranges::iterator_t<tv_t>;
    return std::is_same_v<typename iter::iterator_concept, std::random_access_iterator_tag>;
}(), "transform_view over iota 应保持 random_access iterator_concept");
*/

// =============================================================================
// TODO [必做] 5：层 6 验证 — 完整管道端到端
// =============================================================================
// 层 6 完成后取消注释：

/*
static_assert([] {
    // 完整管道：iota(1,11) | transform(x*x) | take(5) | to<vector<int>>()
    auto v = my::views::iota(1, 11)
           | my::views::transform([](int x){ return x * x; })
           | my::views::take(5)
           | my::ranges::to<std::vector<int>>();
    return v == std::vector<int>{1, 4, 9, 16, 25};
}(), "完整管道结果应为 {1, 4, 9, 16, 25}");
*/

// =============================================================================
// TODO [进阶] 1：std::ranges::copy 消费 mini-ranges view（stdlib 互操作）
// =============================================================================
// 层 5 完成后取消注释：

/*
static_assert([] {
    auto my_view = my::views::iota(1, 6)
                 | my::views::transform([](int x){ return x * 2; });
    std::vector<int> out;
    // std::ranges::copy 通过 my_view.begin()/end() 消费 mini-ranges view
    std::ranges::copy(my_view, std::back_inserter(out));
    return out == std::vector<int>{2, 4, 6, 8, 10};
}(), "stdlib std::ranges::copy 应能消费 mini-ranges view");
*/

// =============================================================================
// TODO [进阶] 2：filter_view — __non_propagating_cache + begin() 非 const
// =============================================================================
// 实现 my_ranges/05_adaptors.hpp 中的 filter_view 骨架（进阶 5f）后在此验证

// =============================================================================
// TODO [进阶] 3：enumerate_view — borrowed 传播
// =============================================================================
// 当底层 V 是 borrowed_range 时，enumerate_view<V>::enable_borrowed_range = true

// =============================================================================
// TODO [进阶] 4：iterator_concept 双轨验证（transform_view）
// =============================================================================
// 验证 iterator_concept（C++20 新轨）vs iterator_category（C++17 旧轨）的不同值

// =============================================================================
// main
// =============================================================================

int main()
{
    std::puts("CAPSTONE4: fill 6 layers to run full pipeline");

    // 完成所有 TODO 并取消注释上方验证块后，运行完整管道：
    //
    // auto v = my::views::iota(1, 11)
    //        | my::views::transform([](int x){ return x * x; })
    //        | my::views::take(5)
    //        | my::ranges::to<std::vector<int>>();
    // assert((v == std::vector<int>{1, 4, 9, 16, 25}));
    // std::puts("CAPSTONE4: pipeline OK — {1, 4, 9, 16, 25}");

    return 0;
}
