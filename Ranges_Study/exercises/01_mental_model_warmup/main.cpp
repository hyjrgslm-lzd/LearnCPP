// ============================================================
// 对应章节：../../01-心智模型.md
// 小节：六个最重要的对象 + 三条 view 语义公理 + 三类特殊 range
// C++ 标准要求：C++20（static_assert 区全部在 C++20 范围内）
//
// 预期运行输出（骨架阶段）：
//   （无输出；int main() 直接 return 0）
//
// 本练习目标：
//   用 static_assert 验证心智模型中六个核心对象的 concept 归属，
//   并通过三条 view 语义公理（O(1) move / copy / destroy）加深理解。
//   所有 TODO 对应 01-心智模型.md 的"六个最重要的对象"与
//   "三条 view 语义公理"两节。
// ============================================================

#include <ranges>
#include <iterator>
#include <vector>
#include <array>
#include <string_view>
#include <span>
#include <algorithm>
#include <functional>

int main() {

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1：range concept 验证
    //   验证以下类型是否满足 std::ranges::range：
    //     - std::vector<int>
    //     - std::string_view
    //     - std::array<int, 5>
    //     - int（原生类型，不是 range）
    //   提示：static_assert(std::ranges::range<T>); 或
    //         static_assert(!std::ranges::range<T>);
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2：view concept 验证
    //   验证以下类型是否满足 std::ranges::view：
    //     - std::string_view                         → 是 view
    //     - std::span<int>                           → 是 view
    //     - std::ranges::iota_view<int,int>          → 是 view
    //     - std::vector<int>                         → 不是 view（O(n) destroy）
    //   提示：view concept 要求 range + enable_view + movable
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3：iterator + sentinel 分离验证
    //   对无界 iota_view（std::views::iota(0)）验证：
    //     - begin() 的类型是某种 iota iterator（不是 unreachable_sentinel_t）
    //     - end() 的类型是 std::unreachable_sentinel_t
    //     - begin/end 类型不同（即不满足 common_range）
    //   提示：使用 decltype(r.begin()) / decltype(r.end()) + std::same_as<>
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4：range adaptor closure 验证
    //   构造一个 range adaptor closure 对象，验证：
    //     (a) views::transform(f) 是一个 range_adaptor_closure
    //     (b) v | closure 与 views::transform(v, f) 产出相同结果
    //     (c) 两个 closure 可以用 | 组合成新的 closure（管道预组合）
    //   提示：std::ranges::range_adaptor_closure<T> concept（C++23）
    //         或直接验证 | 运算符能编译通过即可
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 5：CPO / niebloid 验证
    //   验证 std::ranges::begin 是函数对象（不是函数模板）：
    //     - std::ranges::begin 的类型不是函数指针
    //     - 可以把 std::ranges::sort 作为值传递（赋给 auto 变量）
    //     - std::ranges::sort(v) 能正确排序 std::vector<int>
    //   提示：auto cpo = std::ranges::begin; 看编译是否通过
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 6：projection 验证
    //   定义 struct Person { std::string name; int age; };
    //   用 std::ranges::sort(people, std::less{}, &Person::age) 排序，
    //   验证排序结果与手写 lambda 比较器等价。
    //   再用 std::ranges::find_if + projection 找出第一个 age > 28 的人。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 7：三条 view 语义公理（O(1) move/copy/destroy）
    //   (a) 构造一个 iota_view<int,int>，移动它，验证移动后原对象"空"
    //       但移动操作本身是 O(1)（不依赖元素数量）
    //   (b) 构造 string_view，拷贝它，验证拷贝后两者指向同一数据
    //       （O(1) copy：不复制字符数据）
    //   (c) 用 static_assert 验证 std::vector<int> 不是 view
    //       （因为 destroy 是 O(n)）
    //   提示：std::ranges::view<T> 判断是否满足 view concept
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1：borrowed_range 与 dangling 保护
    //   (a) 验证 string_view 和 span<int> 是 borrowed_range
    //   (b) 验证 vector<int> 不是 borrowed_range
    //   (c) 写一个函数返回临时 vector，把返回值传给 ranges::find，
    //       用 static_assert 验证返回类型是 std::ranges::dangling
    //   提示：std::same_as<decltype(it), std::ranges::dangling>
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2：iterator_concept vs iterator_category 双轨
    //   为无界 iota_view 的 iterator：
    //     - 检查 iterator_concept（应为 random_access_iterator_tag）
    //     - 检查 iterator_category（C++17 遗留系统的标签）
    //   对比两者是否相同，思考为何某些 view 迭代器（如 zip_view）
    //   的两个标签会不同。
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 3：sized_range 与 common_range 对比
    //   填写下表（用 static_assert 验证）：
    //     类型                              sized  common  view
    //     -----------------------------------------------
    //     vector<int>                        ?      ?      ?
    //     iota_view<int,int>（有界）          ?      ?      ?
    //     iota_view<int,unreachable_sentinel> ?      ?      ?
    //     filter_view<iota_view<int,int>,..>  ?      ?      ?
    //   结论：filter_view 为何不是 sized_range？
    // ══════════════════════════════════════════════════════

    return 0;
}

// ---- static_assert 验证区 ----
// 下列断言在 TODO 未填写时保持注释状态；
// 完成 TODO 后逐一取消注释以验证。

// // [必做 1] range concept
// static_assert( std::ranges::range<std::vector<int>>);
// static_assert( std::ranges::range<std::string_view>);
// static_assert( std::ranges::range<std::array<int,5>>);
// static_assert(!std::ranges::range<int>);

// // [必做 2] view concept
// static_assert( std::ranges::view<std::string_view>);
// static_assert( std::ranges::view<std::span<int>>);
// static_assert( std::ranges::view<std::ranges::iota_view<int,int>>);
// static_assert(!std::ranges::view<std::vector<int>>);

// // [必做 3] sentinel 分离
// static_assert(!std::ranges::common_range<std::ranges::iota_view<int,std::unreachable_sentinel_t>>);
// static_assert( std::same_as<
//     decltype(std::views::iota(0).end()),
//     std::unreachable_sentinel_t>);

// // [进阶 1] borrowed_range
// static_assert( std::ranges::borrowed_range<std::string_view>);
// static_assert( std::ranges::borrowed_range<std::span<int>>);
// static_assert(!std::ranges::borrowed_range<std::vector<int>>);
