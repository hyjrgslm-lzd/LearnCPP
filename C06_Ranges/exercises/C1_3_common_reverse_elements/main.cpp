// 模块 C1 · 练习 C1-3：common / reverse / elements / keys / values
// 提案：P0896R4（C++20 common_view、reverse_view、elements_view）
// 标准：C++26
//
// 预期输出（空白 main，仅 static_assert）：
//   （无输出，静默通过）
//
// 完成必做 TODO 后预期输出：
//   sum 1..10 = 55
//   reversed vector: 5 4 3 2 1
//   keys:   Alice Bob Carol
//   values: 95 87 91
//   ids:    1 2 3

#include <ranges>
#include <algorithm>
#include <numeric>
#include <vector>
#include <list>
#include <map>
#include <tuple>
#include <string>
#include <iostream>

int main()
{
    // TODO [必做] 1: views::common — 桥接 C++17 算法
    //   构造 views::iota(1) | views::take(10)，验证它不是 common_range，
    //   再用 views::common 包装，传给 std::accumulate，确认结果 55。

    // TODO [必做] 2: views::reverse — 迭代器概念继承
    //   对 vector<int>{1,2,3,4,5} 取 reverse，打印 5 4 3 2 1。
    //   用 static_assert 验证 reverse_view 的迭代器是 random_access_iterator。
    //   尝试对 forward_list 取 reverse（注释掉，确认 concept 会阻止编译）。

    // TODO [必做] 3: views::keys / views::values 作用于 map
    //   用 map<string,int> 打印所有 key 和所有 value。
    //   用 static_assert 验证 keys == elements<0>（同一类型）。

    // TODO [必做] 4: views::elements<N> 作用于 vector<tuple<int,string,double>>
    //   分别取 elements<0>（int 列）和 elements<1>（string 列）并打印。

    // TODO [必做] 5: borrowed_range + dangling
    //   声明一个返回 map 右值的函数，调用 ranges::find(get_map() | views::values, 2)，
    //   用 static_assert 验证返回类型是 ranges::dangling。

    // TODO [进阶] 1: 验证 common_view 对已是 common_range 的输入是 no-op：
    //   vector<int> 已经是 common_range，套 views::common 后 begin/end 类型不变。

    // TODO [进阶] 2: 解释 views::elements<N> 依赖 tuple 协议（get<N>）：
    //   自定义一个满足 tuple_size / tuple_element / get<N> 的类型，
    //   验证它可以被 elements<0> 投影。

    return 0;
}

// ── static_assert 验证区 ──────────────────────────────────────────────────
// 完成必做 1-5 后，把下列 assert 从注释中解开并确认编译通过。

// static_assert([](){
//     // common
//     auto r  = std::views::iota(1) | std::views::take(10);
//     static_assert(!std::ranges::common_range<decltype(r)>);
//     auto cr = r | std::views::common;
//     static_assert( std::ranges::common_range<decltype(cr)>);
//
//     // reverse preserves random_access
//     std::vector<int> v = {1,2,3};
//     auto rv = v | std::views::reverse;
//     static_assert(std::random_access_iterator<decltype(rv.begin())>);
//
//     // keys == elements<0>
//     std::map<std::string,int> m = {{"a",1}};
//     static_assert(std::same_as<
//         decltype(m | std::views::keys),
//         decltype(m | std::views::elements<0>)>);
//
//     // values_view is NOT borrowed_range (map is not)
//     static_assert(!std::ranges::borrowed_range<std::map<std::string,int>>);
//     auto vv = m | std::views::values;
//     static_assert(!std::ranges::borrowed_range<decltype(vv)>);
//     return true;
// }());
