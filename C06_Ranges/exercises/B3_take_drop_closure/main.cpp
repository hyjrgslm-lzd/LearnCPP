// 练习 B-3：take / drop 家族与管道组合
// 对应章节：03-模块B-基础适配器与管道.md § 练习 B-3
// 提案编号：P0896R4（C++20 ranges 核心合入）
//            P1035R7（take_while / drop_while / drop 适配器正式加入）
//            P2387R3（range_adaptor_closure 正式化，operator| 组合语义）
// C++ 标准：C++20（-std=c++20 / /std:c++20）
//
// 预期输出（骨架阶段，全部 TODO 未填写时）：
//   （无输出，空 main 返回 0，编译通过即为阶段一目标）
//
// 填写 TODO 后预期输出示例：
//   take(3) sized=true  common=true  ra=true
//   drop(2) sized=true  ra=true
//   take_while sized=false common=false
//   drop_while sized=false
//   bounded iota: 1 2 3 4 5 6 7 8 9 10
//   pipeline same_as: r1==r2==r3 ✓

#include <ranges>
#include <vector>
#include <list>
#include <iostream>
#include <concepts>
#include <print>

int main() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // ──────────────────────────────────────────────────────
    // TODO [必做] 1：take(n) 对 random_access + sized 底层的属性
    //   构造 tv = v | std::views::take(3)。
    //   用 static_assert 验证：
    //     - std::ranges::sized_range<decltype(tv)>
    //     - std::ranges::common_range<decltype(tv)>
    //     - std::random_access_iterator<decltype(tv.begin())>
    //   输出 tv 的相关属性标志（可用 if constexpr 打印 true/false）。
    // ──────────────────────────────────────────────────────

    auto tv = v | std::views::take(3);

    static_assert(std::ranges::sized_range<decltype(tv)>);
    static_assert(std::ranges::common_range<decltype(tv)>);
    static_assert(std::random_access_iterator<decltype(tv.begin())>);


    // ──────────────────────────────────────────────────────
    // TODO [必做] 2：drop(n) 对 random_access + sized 底层
    //   构造 dv = v | std::views::drop(2)。
    //   用 static_assert 验证：
    //     - std::ranges::sized_range<decltype(dv)>
    //     - std::random_access_iterator<decltype(dv.begin())>
    //   思考：drop 对 random_access + sized range 为何是 O(1) 跳过？
    // ──────────────────────────────────────────────────────

    auto dv = v | std::views::drop(2);

    static_assert(std::ranges::sized_range<decltype(dv)>);
    static_assert(std::random_access_iterator<decltype(dv.begin())>);


    // ──────────────────────────────────────────────────────
    // TODO [必做] 3：drop(n) 对 bidirectional（list）
    //   声明 std::list<int> lst{1, 2, 3, 4, 5}。
    //   构造 dl = lst | std::views::drop(2)。
    //   用 static_assert 验证：
    //     - std::bidirectional_iterator<decltype(dl.begin())>
    //     - !std::random_access_iterator<decltype(dl.begin())>
    //   思考：drop 对 forward range 的 begin() 是 O(n) 扫描。
    // ──────────────────────────────────────────────────────
    std::list<int> lst{1, 2, 3, 4, 5};
    auto dl = lst | std::views::drop(2);
    static_assert(std::bidirectional_iterator<decltype(dl.begin())>);
    static_assert(!std::random_access_iterator<decltype(dl.begin())>);

    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [必做] 4：take_while(pred) — 非 sized，非 common
    //   构造 tw = v | std::views::take_while([](int x){ return x < 4; })。
    //   用 static_assert 验证：
    //     - !std::ranges::sized_range<decltype(tw)>
    //     - !std::ranges::common_range<decltype(tw)>
    //     - std::random_access_iterator<decltype(tw.begin())>
    //       （iterator_concept 透传，但 end() 是 sentinel）
    // ──────────────────────────────────────────────────────

    auto tw = v | std::views::take_while([](int x){ return x < 4; });
    static_assert(!std::ranges::sized_range<decltype(tw)>);
    static_assert(!std::ranges::common_range<decltype(tw)>);
    static_assert(std::random_access_iterator<decltype(tw.begin())>);


    // ──────────────────────────────────────────────────────
    // TODO [必做] 5：drop_while(pred) — begin() 非 const，非 sized
    //   构造 dw = v | std::views::drop_while([](int x){ return x < 3; })。
    //   用 static_assert 验证 !std::ranges::sized_range<decltype(dw)>。
    //   取消注释下面两行，观察编译失败（begin() 非 const）：
    //
    // const auto const_dw = dw;
    // for (int x : const_dw) { (void)x; }   // 编译失败
    //
    //   与练习 B-2 filter 的 const 迭代失败对比：根源相同（begin() 写缓存）。
    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [必做] 6：无界 iota + take → 有界
    //   构造 bounded = std::views::iota(1) | std::views::take(10)。
    //   用 static_assert 验证 std::ranges::sized_range<decltype(bounded)>。
    //   遍历 bounded 输出所有元素（预期：1 2 3 ... 10）。
    //   说明：无界 iota 本身非 sized_range；take 在 random_access 底层
    //   上产出 sized + common 的 take_view（LWG DR 应用后）。
    // ──────────────────────────────────────────────────────

    auto bounded = std::views::iota(1) | std::views::take(10);
    static_assert(!std::ranges::sized_range<decltype(bounded)>);


    // ──────────────────────────────────────────────────────
    // TODO [必做] 7：管道语法三种等价形式（range_adaptor_closure 组合）
    //   在下方填写三种等价写法，并用 static_assert(std::same_as<...>) 验证
    //   r1、r2、r3 类型完全相同。
    //   注意：is_odd / sq 必须是同一个 lambda 对象，不能重写三次。
    //
    std::vector<int> v2{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto is_odd = [](int x) { return x % 2 != 0; };
    auto sq     = [](int x) { return x * x; };

    // 方式 A：直接管道（左折叠）
    // auto r1 = v2 | std::views::filter(is_odd)
    //              | std::views::transform(sq)
    //              | std::views::take(3);

    // 方式 B：预先组合 closure（closure | closure → 新 closure，P2387R3）
    // auto pipeline = std::views::filter(is_odd)
    //               | std::views::transform(sq)
    //               | std::views::take(3);
    // auto r2 = v2 | pipeline;

    // 方式 C：函数调用语法（等价，views::filter(v, p) 走相同的 all 规范化路径）
    // auto r3 = std::views::take(
    //             std::views::transform(
    //               std::views::filter(v2, is_odd), sq), 3);

    // 验证三者类型相同：
    // static_assert(std::same_as<decltype(r1), decltype(r2)>);
    // static_assert(std::same_as<decltype(r1), decltype(r3)>);
    // 展开类型：take_view{ transform_view{ filter_view{ ref_view{v2}, is_odd }, sq }, 3 }
    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [进阶] A：drop_while 的 begin() 缓存类比 filter
    //   在注释里说明：drop_while 与 filter 的缓存机制相同点与关键差异。
    //   相同点：optional<iterator> 写入决定 begin() 非 const。
    //   差异：drop_while 找到起点后 ++it 直接推进（不再检查条件）；
    //          filter 的每次 ++it 都需扫描到下一个满足条件的元素。
    // ──────────────────────────────────────────────────────


    // ──────────────────────────────────────────────────────
    // TODO [进阶] B：take_while 与 filter 的语义对比
    //   构造以下两个管道并分别输出结果，说明语义根本不同：
    //   管道 X：std::views::iota(1,11) | views::transform(sq)
    //           | views::take_while([](int x){ return x < 50; })
    //   管道 Y：std::views::iota(1,11) | views::filter(is_odd) | views::transform(sq)
    //   思考：take_while 是前缀截断，filter 是全局跳过式筛选。
    // ──────────────────────────────────────────────────────


    return 0;
}

// ---- static_assert 验证区 ----
// 以下断言在填写对应 TODO 后取消注释逐步验证。
// 骨架阶段全部注释掉，保证空骨架可编译。

// 需要 tv / dv / dl / tw / dw / bounded 在 main 内才能启用：
 //static_assert(std::ranges::sized_range<decltype(tv)>);
 //static_assert(std::ranges::common_range<decltype(tv)>);
 //static_assert(std::random_access_iterator<decltype(tv.begin())>);
 //static_assert(std::ranges::sized_range<decltype(dv)>);
 //static_assert(std::bidirectional_iterator<decltype(dl.begin())>);
 //static_assert(!std::random_access_iterator<decltype(dl.begin())>);
 //static_assert(!std::ranges::sized_range<decltype(tw)>);
 //static_assert(!std::ranges::common_range<decltype(tw)>);
 //static_assert(std::random_access_iterator<decltype(tw.begin())>);
 //static_assert(!std::ranges::sized_range<decltype(dw)>);
 //static_assert(std::ranges::sized_range<decltype(bounded)>);
 //static_assert(std::ranges::view<decltype(tv)>);
 //static_assert(std::ranges::view<decltype(dv)>);
 //static_assert(std::ranges::view<decltype(tw)>);
 //static_assert(std::ranges::view<decltype(dw)>);
