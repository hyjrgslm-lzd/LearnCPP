// 章节：模块 G — 自行实现视图
// 小节：G-1 my_take_view — 最小 view_interface 实战
// 提案：P0896R4（view_interface 设计），P2325R3（view 不必 default_initializable）
// C++ 标准：C++26
//
// 预期输出（实现完成后）：
//   G-1 all assertions passed.
//
// 默认输出（骨架阶段，空实现可编译）：
//   G-1 skeleton compiled.

#include <ranges>
#include <iterator>
#include <concepts>
#include <vector>
#include <list>
#include <cassert>
#include <iostream>

// ── my_take_view<V> ───────────────────────────────────────────────────────────
//
// 设计要点：
//   - 继承 view_interface<my_take_view<V>>，CRTP 注入 empty()/front()/operator bool() 等
//   - 内部存储 V base_ 和计数 count_
//   - begin() 直接返回底层 begin()
//   - end() 分两条路径：
//       路径 A（random_access + sized）：返回 begin() + min(count_, size)
//       路径 B（其他）：返回 default_sentinel，搭配 counted_iterator 包装 begin()
//   - opt-in：继承 view_interface 后 enable_view<my_take_view<V>> 自动为 true
//     （前提：类型满足 movable + 能被 range 消费）
//
// CTAD 推导指引：
//   template<viewable_range R>
//   my_take_view(R&&, range_difference_t<R>) -> my_take_view<views::all_t<R>>;
//
// size() 成员：仅在 V 是 sized_range 时启用（requires 约束保护）；
//   需要把 count_ 夹到 [0, size(base_)] 避免负值被强转到 size_t 产生垃圾值。

template <std::ranges::view V>
class my_take_view : public std::ranges::view_interface<my_take_view<V>> {
public:
    my_take_view() = default;

    constexpr my_take_view(V base, std::ranges::range_difference_t<V> n)
        : base_(std::move(base)), count_(n) {}

    // TODO [必做] 1: 实现 begin()
    //   直接返回 std::ranges::begin(base_)
    constexpr auto begin() {
        // TODO [必做] 1: return std::ranges::begin(base_);
        return std::ranges::begin(base_);
    }

    // TODO [必做] 2: 实现 end()，区分两条路径
    //   if constexpr (random_access_range<V> && sized_range<V>):
    //     计算 n = min(count_, distance(base_))，返回 begin() + n
    //   else:
    //     返回 std::default_sentinel
    //   路径 B 下，begin() 应包装为 counted_iterator 才能在 range-for 中被 default_sentinel 正确终止。
    //   提示：counted_iterator 已由 begin() 内部提供时，end() 只需返回 default_sentinel。
    constexpr auto end() {
        // TODO [必做] 2:
        // if constexpr (std::ranges::random_access_range<V> && std::ranges::sized_range<V>) {
        //     auto sz = std::ranges::distance(base_);
        //     auto n  = std::min(count_, sz);
        //     return std::ranges::begin(base_) + n;
        // } else {
        //     return std::default_sentinel;
        // }
        return std::ranges::end(base_); // 占位：不分路径，先让骨架可编译
    }

    // TODO [必做] 3: 实现 size()（requires sized_range<V>）
    //   将 count_ 夹到 [0, size(base_)] 区间，转型为 range_size_t<V> 返回
    constexpr auto size()
        requires std::ranges::sized_range<V>
    {
        // TODO [必做] 3:
        // auto clamped = count_ < 0 ? std::ranges::range_difference_t<V>{0} : count_;
        // auto sz = static_cast<std::ranges::range_difference_t<V>>(std::ranges::size(base_));
        // return static_cast<std::ranges::range_size_t<V>>(std::min(clamped, sz));
        return std::ranges::size(base_); // 占位
    }

    // TODO [进阶] A: 添加 const 重载版本的 begin() / end()
    //   requires std::ranges::range<const V>

    constexpr V base() const& { return base_; }
    constexpr V base() &&     { return std::move(base_); }

private:
    V base_{};
    std::ranges::range_difference_t<V> count_{};
};

// ── 推导指引 ──────────────────────────────────────────────────────────────────
// 作用：让 my_take_view(vec, 3) 自动推导 V = views::all_t<vector<int>&>
// 没有此指引时，构造函数参数 V 无法从 vector<int>& 推导（V 要求 view，vector 不是 view）

template <std::ranges::viewable_range R>
my_take_view(R&&, std::ranges::range_difference_t<R>)
    -> my_take_view<std::views::all_t<R>>;

// ── static_assert 验证区 ──────────────────────────────────────────────────────

// 验证 1：继承 view_interface 后满足 view concept
static_assert(std::ranges::view<my_take_view<std::views::all_t<std::vector<int>&>>>,
              "my_take_view 必须满足 std::ranges::view");

// 验证 2：vector 底层（random_access + sized）→ 应为 common_range
// （注：此 static_assert 在 TODO [必做] 2 实现后才能通过）
// static_assert(std::ranges::common_range<my_take_view<std::views::all_t<std::vector<int>&>>>,
//               "random_access + sized 底层 → common_range");

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    // 骨架阶段：仅验证编译，不运行断言
    // 实现 TODO 后取消注释以下所有测试

    /*
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};

    // 推导指引：V 推导为 ref_view<vector<int>>
    my_take_view tv{v, 4};

    // view concept
    static_assert(std::ranges::view<decltype(tv)>);
    static_assert(std::ranges::range<decltype(tv)>);

    // random_access + sized → common_range + sized_range
    static_assert(std::ranges::common_range<decltype(tv)>);
    static_assert(std::ranges::sized_range<decltype(tv)>);

    // view_interface 注入的成员
    assert(!tv.empty());
    assert(tv);
    assert(tv.front() == 1);
    assert(tv.back()  == 4);
    assert(tv[2]      == 3);

    // 迭代验证
    int expected[] = {1, 2, 3, 4};
    int idx = 0;
    for (int x : tv) assert(x == expected[idx++]);
    assert(idx == 4);

    // count_ 超过 size 时截断
    my_take_view tv2{v, 100};
    assert(tv2.size() == 8u);

    // 路径 B：list 底层（bidirectional，非 random_access + sized）
    std::list<int> lst{10, 20, 30, 40, 50};
    my_take_view tv_lst{lst, 3};
    static_assert(!std::ranges::common_range<decltype(tv_lst)>);
    int n = 0;
    for (int x : tv_lst) { (void)x; ++n; }
    assert(n == 3);

    // 接入标准管道
    auto doubled = tv | std::views::transform([](int x){ return x * 2; });
    int sum = 0;
    for (int x : doubled) sum += x;
    assert(sum == 20); // (1+2+3+4)*2

    std::cout << "G-1 all assertions passed.\n";
    */

    std::cout << "G-1 skeleton compiled.\n";
    return 0;
}
