// 练习 B-1：all / ref_view / owning_view
// 对应章节：03-模块B-基础适配器与管道.md § 练习 B-1
// 提案编号：P0896R4（C++20 ranges 核心合入）、P2415R2（owning_view 引入）
// C++ 标准：C++20（-std=c++20 / /std:c++20）
//
// 预期输出（骨架阶段，全部 TODO 未填写时）：
//   （无输出，空 main 返回 0，编译通过即为阶段一目标）
//
// 填写 TODO 后预期输出示例：
//   ref_view size  = 6      （push_back(6) 后 rv 透过指针观察到修改）
//   owning_view size = 3
//   iota_view pass-through  （已是 view，直接透传）

#include <ranges>
#include <vector>
#include <string>
#include <string_view>
#include <iostream>
#include <concepts>

int main() {
    // ──────────────────────────────────────────────────────
    // TODO [必做] 1：左值路径 → ref_view
    //   声明 std::vector<int> vec{1, 2, 3, 4, 5}，
    //   用 std::views::all(vec) 得到 rv。
    //   用 static_assert + std::same_as 验证 rv 是
    //   std::ranges::ref_view<std::vector<int>>。
    //   验证 rv 是 std::ranges::borrowed_range。
    //   调用 vec.push_back(6)，输出 rv 的 size，观察借用语义。
    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [必做] 2：右值路径 → owning_view（P2415R2）
    //   用 std::views::all(std::vector<int>{10, 20, 30}) 得到 ov。
    //   用 static_assert 验证 ov 是
    //   std::ranges::owning_view<std::vector<int>>。
    //   验证 ov 不是 std::ranges::borrowed_range。
    //   输出 ov 的 size。
    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [必做] 3：已是 view → 直接透传，不再包装
    //   用 std::views::iota(1, 6) 得到 iv，
    //   再调用 std::views::all(iv) 得到 iv_all。
    //   用 static_assert 验证 iv_all 和 iv 的类型完全相同
    //   （std::ranges::iota_view<int, int>）。
    //   输出一行说明透传成功。
    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [必做] 4：viewable_range 概念验证
    //   用 static_assert 验证：
    //     - std::vector<int>& 满足 std::ranges::viewable_range
    //     - std::vector<int>  满足 std::ranges::viewable_range（右值）
    //     - std::ranges::ref_view<std::vector<int>>
    //         满足 std::ranges::viewable_range
    //   思考：为何左值引用和右值容器都满足 viewable_range 但路径不同？
    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [进阶] 1：P2415R2 之前的漏洞
    //   阅读 viewable_range concept 的条件分支（cppreference）。
    //   在注释里说明：为何 P2415R2 之前 views::all(std::vector{1,2,3})
    //   是非法的，owning_view 补入后如何解决右值容器的悬垂问题。
    //   可选：用 std::string_view（本身是 view + borrowed_range）验证
    //   直接透传行为；用右值 std::string 验证得到 owning_view<string>。
    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [进阶] 2：ref_view vs span 的根本区别
    //   在注释里回答：ref_view 和 std::span 都是借用语义，
    //   根本区别是什么？（提示：span 是 contiguous_range，
    //   ref_view 保持底层 range 的迭代器类别）
    //   可选：用 static_assert 对比 ref_view<list<int>> 和 span<int>
    //   的 contiguous_range 属性。
    // ──────────────────────────────────────────────────────


    return 0;
}

// ---- static_assert 验证区 ----
// 以下断言在填写 TODO 后取消注释逐步验证。
// 骨架阶段全部注释掉，保证空骨架可编译。

// 需要在 main 内有 vec / rv / ov / iv / iv_all 才能启用：
// static_assert(std::ranges::view<decltype(rv)>);
// static_assert(std::ranges::borrowed_range<decltype(rv)>);
// static_assert(!std::ranges::borrowed_range<decltype(ov)>);
// static_assert(std::same_as<
//     decltype(rv),
//     std::ranges::ref_view<std::vector<int>>>);
// static_assert(std::same_as<
//     decltype(ov),
//     std::ranges::owning_view<std::vector<int>>>);
// static_assert(std::same_as<decltype(iv_all), std::ranges::iota_view<int,int>>);
