// 练习 B-2：filter / transform 与 iterator 降级
// 对应章节：03-模块B-基础适配器与管道.md § 练习 B-2
// 提案编号：P0896R4（C++20 ranges 核心合入，filter_view / transform_view 设计）
// C++ 标准：C++20（-std=c++20 / /std:c++20）
//
// 预期输出（骨架阶段，全部 TODO 未填写时）：
//   （无输出，空 main 返回 0，编译通过即为阶段一目标）
//
// 填写 TODO 后预期输出示例：
//   layer0 (vector)      : random_access_iterator ✓
//   layer1 (ref_view)    : random_access_iterator ✓
//   layer2 (filter)      : bidirectional_iterator ✓, NOT random_access ✓
//   layer3 (filter+xfrm) : bidirectional_iterator ✓
//   layer4 (xfrm only)   : random_access_iterator ✓
//   消费 l2: 1 9 25

#include <ranges>
#include <vector>
#include <iostream>
#include <concepts>
#include <print>

// 谓词与变换函数——提取为命名变量，保证 lambda 类型唯一
// （不同 lambda 表达式即使源文本一致也是不同闭包类型）
auto is_odd = [](int x) { return x % 2 != 0; };
auto sq     = [](int x) { return x * x; };

int main() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // ──────────────────────────────────────────────────────
    // TODO [必做] 1：layer 0 — vector 原始迭代器类别
    //   用 static_assert 验证 std::vector<int>::iterator
    //   满足 std::random_access_iterator。
    //   （参照点：管道的起点）
    // ──────────────────────────────────────────────────────

    static_assert(std::random_access_iterator<std::vector<int>::iterator>);

    // ──────────────────────────────────────────────────────
    // TODO [必做] 2：layer 1 — ref_view 透传
    //   用 std::views::all(v) 得到 l0。
    //   用 static_assert 验证 l0.begin() 仍是 random_access_iterator。
    // ──────────────────────────────────────────────────────
    auto l0 = std::views::all(v);
    static_assert(std::random_access_iterator<decltype(l0.begin())>);

    // ──────────────────────────────────────────────────────
    // TODO [必做] 3：layer 2 — filter_view 降级
    //   构造 l1 = v | std::views::filter(is_odd)。
    //   用 static_assert 验证：
    //     - l1.begin() 满足 std::bidirectional_iterator
    //     - l1.begin() 不满足 std::random_access_iterator
    //   思考：filter 为何必然降级到 bidirectional？
    // ──────────────────────────────────────────────────────

    auto l1 = v | std::views::filter(is_odd);
    static_assert(std::bidirectional_iterator<decltype(l1.begin())>);
    static_assert(!std::random_access_iterator<decltype(l1.begin())>);

    // ──────────────────────────────────────────────────────
    // TODO [必做] 4：layer 3 — filter + transform，transform 透传
    //   构造 l2 = v | std::views::filter(is_odd) | std::views::transform(sq)。
    //   用 static_assert 验证 l2.begin() 满足 bidirectional_iterator。
    //   遍历 l2 并输出每个元素（预期：1 9 25）。
    // ──────────────────────────────────────────────────────
    auto l2 = v | std::views::filter(is_odd) | std::views::transform(sq);
    static_assert(std::bidirectional_iterator<decltype(l2.begin())>);
    for (int x : l2)
        std::print("{} ", x);
    std::println("");

    // ──────────────────────────────────────────────────────
    // TODO [必做] 5：layer 4 — 仅 transform，保持 random_access
    //   构造 l3 = v | std::views::transform(sq)。
    //   用 static_assert 验证 l3.begin() 满足 random_access_iterator。
    // ──────────────────────────────────────────────────────

    auto l3 = v | std::views::transform(sq);
    static_assert(std::random_access_iterator<decltype(l3.begin())>);


    // ──────────────────────────────────────────────────────
    // TODO [进阶] A：iota 底层 + filter/transform 降级路径
    //   把底层换成 std::views::iota(1, 6)（本身是 random_access）。
    //   观察接 filter 后降为 bidirectional，接 transform 后保持 random_access。
    //   与 vector 底层的结果对比：降级规则与底层无关，只取决于 adaptor。
    // ──────────────────────────────────────────────────────

    auto v2 = std::views::iota(1, 6);
    auto w1 = v2 | std::views::filter(is_odd);
    auto w2 = v2 | std::views::transform(sq);
    static_assert(std::bidirectional_iterator<decltype(w1.begin())>);
    static_assert(std::random_access_iterator<decltype(w2.begin())>);

    // ──────────────────────────────────────────────────────
    // TODO [进阶] B：const 管道 + filter 的编译失败
    //   取消注释下面两行，观察编译错误（filter_view::begin() 非 const）：
    //
    // const auto pipe = v | std::views::filter(is_odd);
    // for (int x : pipe) { (void)x; }   // 编译失败：begin() 要求非 const 对象
    //
    //   读报错中的 "requires non-const" 或 "no matching function" 字样。
    //   思考：C++23 views::as_const（P2278R4）是否解决此问题？答：不解决。
    // ──────────────────────────────────────────────────────




    // ──────────────────────────────────────────────────────
    // TODO [进阶] C：transform 的 const 可调用要求
    //   定义一个带可变捕获的 lambda：
    //     int counter = 0;
    //     auto mutable_fn = [counter](int x) mutable { ++counter; return x*x; };
    //   非 const 管道迭代（OK）：
    //     auto pipe2 = v | std::views::transform(mutable_fn);
    //     for (int x : pipe2) { (void)x; }
    //   然后取消注释下行，观察编译失败：
    //     // const auto cpipe2 = v | std::views::transform(mutable_fn);
    //     // for (int x : cpipe2) { (void)x; }  // 编译失败：const F::operator() 不存在
    //   解决方法：改用无捕获或只读捕获的 lambda。
    // ──────────────────────────────────────────────────────


    return 0;
}

// ---- static_assert 验证区 ----
// 以下断言在填写对应 TODO 后取消注释逐步验证。
// 骨架阶段全部注释掉，保证空骨架可编译。

// static_assert(std::random_access_iterator<std::vector<int>::iterator>);

// 需要 l0 / l1 / l2 / l3 在 main 内才能启用：
// static_assert(std::random_access_iterator<decltype(l0.begin())>);
// static_assert(std::bidirectional_iterator<decltype(l1.begin())>);
// static_assert(!std::random_access_iterator<decltype(l1.begin())>);
// static_assert(std::bidirectional_iterator<decltype(l2.begin())>);
// static_assert(std::random_access_iterator<decltype(l3.begin())>);
// static_assert(std::ranges::view<decltype(l1)>);
// static_assert(std::ranges::view<decltype(l2)>);
