// 模块 C1 · 练习 C1-2：split 的 C++20 vs C++23 差异
// 提案：P0896R4（C++20 split_view 原始版）、P2210R2（C++23 语义改进）
// 标准：C++26（lazy_split 需 C++20+，新 split 需 C++23+）
//
// P2210R2 要点：C++20 views::split 在 C++23 中改名为 views::lazy_split；
//   新 views::split 的子范围保留底层 contiguous 性质，可直接构造 string_view。
//
// 预期输出（空白 main，仅 static_assert）：
//   （无输出，静默通过）
//
// 完成必做 TODO 后预期输出：
//   lazy_split: hello|world|ranges|
//   new split:  hello|world|ranges|

#include <ranges>
#include <string_view>
#include <string>
#include <iostream>
#include <print>

int main()
{
    // TODO [必做] 1: 用 views::lazy_split(',') 切分 "hello,world,ranges"。
    //   内层手动逐字符打印，确认输出 hello|world|ranges|。
    //   用 static_assert 验证子范围是 forward_range 而非 contiguous_range。

    // TODO [必做] 2: 用 C++23 views::split(',') 切分同一字符串。
    //   用 string_view(subr.begin(), subr.end()) 构造每个子串并打印。
    //   用 static_assert 验证子范围是 contiguous_range。

    // TODO [必做] 3: 对比两种子范围类型：
    //   static_assert(!ranges::contiguous_range<lazy_first_type>)
    //   static_assert( ranges::contiguous_range<split_first_type>)
    //   并在注释里解释原因（代理迭代器 vs 透传底层迭代器）。

    // TODO [必做] 4: 在注释里说明 P2210R2 的处理方式：
    //   为什么选择"改名保留"而非"删除旧版本"，对代码库迁移有何影响？

    // TODO [进阶] 1: 用 lazy_split 切分 forward_list<int>，分隔值为 0，
    //   打印各子段，验证子范围迭代器是 forward_iterator。

    // TODO [进阶] 2: 在注释里回答：
    //   std::string(subr.begin(), subr.end()) 对 lazy_split 子范围能否编译？
    //   std::string_view(subr.begin(), subr.end()) 呢？说明原因。

    return 0;
}

// ── static_assert 验证区 ──────────────────────────────────────────────────
// 完成必做 1-3 后，把下列 assert 从注释中解开并确认编译通过。

// static_assert([](){
//     std::string_view sv = "a,b,c";
//     // lazy_split：子范围仅是 forward_range
//     auto lazy_parts  = sv | std::views::lazy_split(',');
//     auto lazy_first  = *lazy_parts.begin();
//     static_assert(!std::ranges::contiguous_range<decltype(lazy_first)>);
//     static_assert( std::ranges::forward_range  <decltype(lazy_first)>);
//     // new split（C++23）：子范围保留 contiguous 性质
//     auto split_parts = sv | std::views::split(',');
//     auto split_first = *split_parts.begin();
//     static_assert(std::ranges::contiguous_range<decltype(split_first)>);
//     return true;
// }());
