// 模块 D — 练习 D-1：zip / zip_transform / adjacent / slide / chunk / stride
// 章节：06-模块D-C++23高阶视图与协程桥.md §练习 D-1
// 提案：P2321R2 (zip/zip_transform), P2442R1 (chunk/slide/chunk_by), P1899R3 (stride)
// C++ 标准：C++23（本练习涉及的所有视图均为 C++23 新增）
//
// 预期输出（填完 TODO 后）：
//   zip(a, b, c): (1,1.5,10) (2,2.5,20) (3,3.5,30)
//   zip_transform(plus, a, b): 2.5 4.5 6.5 8.5
//   adjacent<2>: (1,2) (2,3) (3,4) (4,5)
//   adjacent<3>: (1,2,3) (2,3,4) (3,4,5)
//   slide(3): [123] [234] [345]
//   chunk(3): [123] [456] [7]
//   stride(2): 1 3 5 7
//
// 关键概念：
//   - zip_view 的 proxy reference 导致 iterator_category = input_iterator_tag
//     但 iterator_concept = random_access_iterator_tag（双轨机制，见 01-心智模型.md）
//   - adjacent<N>：静态窗口，产出 tuple<T&,...>，可结构化绑定
//   - slide(n)：动态窗口，产出 subrange，不可结构化绑定
//   - chunk(n)：非重叠分块；stride(n)：跳步取样

#include <ranges>
#include <vector>
#include <tuple>
#include <functional>
#include <iostream>

int main() {
    // ============================================================
    // TODO [必做] 1: 观察 views::zip 的基本行为
    //   构造 a={1,2,3,4}, b={1.5,2.5,3.5,4.5}, c={10,20,30}
    //   用 views::zip(a, b, c) 遍历，打印每个 tuple 元素
    //   验证：产出长度 = min(|a|,|b|,|c|) = 3
    // ============================================================

    // TODO [必做] 2: 验证 zip_view 的 iterator 双轨
    //   auto z2 = views::zip(a, b);
    //   用 static_assert 分别验证：
    //     - iterator_concept == random_access_iterator_tag
    //     - iterator_category == input_iterator_tag
    //   说出原因：proxy reference (tuple<int&, double&>) 导致降级
    // ============================================================

    // TODO [必做] 3: 使用 views::zip_transform
    //   用 zip_transform(std::plus<>{}, a, b) 逐元素求和
    //   再写等价的 zip + transform 版本对比
    //   预期输出：2.5 4.5 6.5 8.5
    // ============================================================

    // TODO [必做] 4: 观察 views::adjacent<2> 和 adjacent<3>
    //   vector<int> v = {1,2,3,4,5}
    //   adjacent<2>：产出 tuple<int&,int&>，大小 = size-1
    //   adjacent<3>：产出 tuple<int&,int&,int&>，大小 = size-2
    //   用 static_assert 验证 tuple_size == 3（对 adjacent<3>）
    // ============================================================

    // TODO [必做] 5: 对比 views::slide(3) 与 adjacent<3>
    //   slide 产出 subrange，不能结构化绑定
    //   adjacent 产出 tuple，可以 auto [x,y,z] = ...
    //   用 static_assert 对比两者的 range_value_t
    // ============================================================

    // TODO [必做] 6: 观察 views::chunk(3) 的非重叠分块
    //   v = {1,2,3,4,5,6,7}，chunk(3) 产出 [123][456][7]
    //   最后一块可能不足 3 个，不丢弃
    // ============================================================

    // TODO [必做] 7: 观察 views::stride(2) 的跳步取样
    //   v = {1,2,3,4,5,6,7}，stride(2) 产出 1 3 5 7
    //   用 static_assert 验证 random_access_range（vector 底层 O(1) advance）
    // ============================================================

    // TODO [进阶] 1: 对比 adjacent_transform<2> 与手写差分
    //   v | adjacent_transform<2>([](int a, int b){ return b-a; })
    //   应产出 1 1 1 1（相邻差）
    // ============================================================

    // TODO [进阶] 2: 用 std::list 替换 vector 作为 stride 底层
    //   用 static_assert 验证 stride_view 迭代器不再是 random_access_iterator
    //   （list 是 bidirectional_range，advance 退化为 O(k*n)）
    // ============================================================

    // TODO [进阶] 3: views::zip(a, views::repeat(0)) 模拟全零配对
    //   对比直接用 zip_transform(f, a, zeros) 的写法
    // ============================================================

    // TODO [进阶] 4: 验证 zip 不是笛卡尔积
    //   views::zip({1,2}, {3,4}) 产出 2 个元素，不是 4 个
    //   笛卡尔积需要 views::cartesian_product（模块 A 练习 A-3）
    // ============================================================

    std::cout << "D1 skeleton — fill TODOs to observe zip/adjacent/chunk/stride\n";
    return 0;
}

// ============================================================
// 静态验证区（编译期断言，填完 TODO 后打开）
// ============================================================
// 填完后解注释以下 static_assert：

// #include <list>
// static_assert(std::ranges::random_access_range<
//     decltype(std::vector<int>{} | std::views::stride(2))>);
// static_assert(!std::ranges::random_access_range<
//     decltype(std::list<int>{} | std::views::stride(2))>);
