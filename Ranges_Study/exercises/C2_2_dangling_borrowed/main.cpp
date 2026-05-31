// 模块 C2 · 练习 C2-2：ranges::dangling 与 enable_borrowed_range 自定义
// 提案：P0896R4（ranges::dangling、borrowed_range 初始设计）
//        P1739R4（borrowed_range 细化，哪些标准类型特化为 true）
//        P2017R1（conditionally borrowed）
//        P1207R4（iota_view borrowed 属性）
// 标准：C++26
//
// 预期输出（空白 main，仅 static_assert）：
//   （无输出，静默通过）
//
// 完成必做 TODO 后预期输出：
//   found 'r' at offset: 6
//   found 8 at index: 2

#include <ranges>
#include <algorithm>
#include <vector>
#include <string_view>
#include <span>
#include <iostream>

// TODO [必做] 3（自定义类型定义区）：
// 在此处定义 my_borrowed_span<T>，实现 begin()/end()，
// 然后特化 std::ranges::enable_borrowed_range<my_borrowed_span<T>> = true。

int main()
{
    // TODO [必做] 1: ranges::dangling 验证
    //   声明 make_vec() 返回右值 vector<int>，
    //   auto it = ranges::find(make_vec(), 4)
    //   用 static_assert 验证 it 类型是 ranges::dangling。
    //   注释掉 *it 那行，说明 dangling 没有 operator* 是编译期保护。

    // TODO [必做] 2: string_view 是 borrowed_range
    //   auto it_sv = ranges::find(std::string_view{"hello ranges"}, 'r')
    //   用 static_assert 验证类型不是 dangling，且是 contiguous_iterator。
    //   打印偏移量，确认为 6。

    // TODO [必做] 3: 自定义 my_borrowed_span<T>
    //   实现 ptr + len 的轻量 span，并特化 enable_borrowed_range = true。
    //   用 ranges::find(my_borrowed_span<int>{arr,6}, 8) 对右值实例取迭代器，
    //   验证返回类型是 int* 而非 dangling，打印找到的下标。

    // TODO [必做] 4: borrowed_iterator_t / borrowed_subrange_t 类型别名
    //   static_assert(same_as<borrowed_iterator_t<string_view>, string_view::iterator>)
    //   static_assert(same_as<borrowed_iterator_t<vector<int>>,  ranges::dangling>)
    //   static_assert(same_as<borrowed_subrange_t<my_borrowed_span<int>>,
    //                         ranges::subrange<int*>>)

    // TODO [必做] 5: 常见 borrowed view 一览
    //   用 static_assert 验证以下各类型的 borrowed_range 属性：
    //     string_view→true, span<int>→true, iota_view<int,int>→true
    //     vector<int>→false, string→false
    //     ref_view（views::all(左值v)）→true
    //     owning_view（views::all(右值vector)）→false

    // TODO [进阶] 1: ranges::copy 的 in_out_result 结构化绑定
    //   auto [in_end, out_end] = ranges::copy(src, dst.begin())
    //   验证 in_end == src.end()，out_end == dst.end()。

    // TODO [进阶] 2: ranges::equal_range 返回 subrange
    //   在已排序 vector 上调用 ranges::equal_range(v, 3)，
    //   打印找到的子范围并验证 size() == 3。

    return 0;
}

// ── static_assert 验证区 ──────────────────────────────────────────────────
// 完成必做 2-5 后，把下列 assert 从注释中解开并确认编译通过。

// static_assert(std::ranges::borrowed_range<std::string_view>);
// static_assert(std::ranges::borrowed_range<std::span<int>>);
// static_assert(std::ranges::borrowed_range<std::ranges::iota_view<int,int>>);
// static_assert(!std::ranges::borrowed_range<std::vector<int>>);
// static_assert(!std::ranges::borrowed_range<std::string>);
//
// static_assert(std::same_as<
//     std::ranges::borrowed_iterator_t<std::vector<int>>,
//     std::ranges::dangling>);
