// 章节：模块 H — 高级实现模式
// 小节：H-2 common_iterator + iter_move/iter_swap 定制
// 提案：P0896R4（common_iterator 设计），P2321R2（zip_view iter_move/iter_swap），P2494R2（ranges::as_const/as_rvalue）
// C++ 标准：C++26
//
// 预期输出（实现完成后）：
//   sum(1..10) = 55
//   sorted by key:
//     1 -> 1.1
//     1 -> 1.2
//     2 -> 2.2
//     3 -> 3.3
//     4 -> 4.4
//     5 -> 5.5
//     6 -> 6.6
//     9 -> 9.9
//   iter_move 类型验证通过
//   H-2 all assertions passed.
//
// 默认输出（骨架阶段）：
//   H-2 skeleton compiled.

#include <ranges>
#include <numeric>
#include <algorithm>
#include <iterator>
#include <tuple>
#include <variant>
#include <vector>
#include <cassert>
#include <iostream>

// ── my_common_iterator<I, S> ──────────────────────────────────────────────────
//
// 作用：将 begin/end 类型不同的 range 暴露为 begin/end 同类型的 common_range，
//   使其可以传给 C++17 算法（std::accumulate、std::copy 等）。
//
// 内部结构：std::variant<I, S>
//   - 作为 begin 时持有 I；作为 end 时持有 S
//   - operator*()：从 variant 取出 I 解引用
//   - operator++()：推进 I
//   - operator==(begin, end)：检测 I 是否已到达 S 的位置
//
// TODO [必做] 1: 实现内部 var_（variant<I, S>）成员和构造函数
// TODO [必做] 2: 实现 operator*()
// TODO [必做] 3: 实现 operator++()
// TODO [必做] 4: 实现 operator==(const my_common_iterator&, const my_common_iterator&)
//   四种情况：(I,I)/(I,S)/(S,I)/(S,S)，其中 (I,I) 通常不出现在 common_range 的使用场景

template<std::input_or_output_iterator I, std::sentinel_for<I> S>
    requires (!std::same_as<I, S> && std::copyable<I>)
class my_common_iterator {
    std::variant<I, S> var_;

public:
    // 从 I 构造（begin 端）
    constexpr explicit my_common_iterator(I i) : var_(std::in_place_index<0>, std::move(i)) {}
    // 从 S 构造（end 端）
    constexpr explicit my_common_iterator(S s) : var_(std::in_place_index<1>, std::move(s)) {}

    // TODO [必做] 2: operator*()
    //   return *std::get<I>(var_);
    constexpr decltype(auto) operator*() const {
        // TODO [必做] 2:
        return *std::get<I>(var_);
    }

    // TODO [必做] 3: operator++()
    //   ++std::get<I>(var_); return *this;
    constexpr my_common_iterator& operator++() {
        // TODO [必做] 3:
        ++std::get<I>(var_);
        return *this;
    }
    constexpr void operator++(int) { ++*this; }

    // TODO [必做] 4: operator==
    //   对 (I,S) 和 (S,I) 检查 I 到达 S 的位置；(S,S) 返回 true；(I,I) 返回 false
    friend constexpr bool operator==(const my_common_iterator& a,
                                     const my_common_iterator& b) {
        // TODO [必做] 4:
        if (a.var_.index() == 1 && b.var_.index() == 1) return true;
        if (a.var_.index() == 0 && b.var_.index() == 1)
            return std::get<I>(a.var_) == std::get<S>(b.var_);
        if (a.var_.index() == 1 && b.var_.index() == 0)
            return std::get<I>(b.var_) == std::get<S>(a.var_);
        return false;
    }

    // iterator traits（供 std::accumulate 等 C++17 算法使用）
    using value_type      = std::iter_value_t<I>;
    using difference_type = std::iter_difference_t<I>;
    using iterator_category = std::input_iterator_tag;
    using reference       = std::iter_reference_t<I>;
    using pointer         = void;
};

// ── ZipIterator<It1, It2>：proxy reference + iter_move + iter_swap ─────────────
//
// proxy reference：operator*() 返回 tuple<T1&, T2&>（prvalue），不是真实左值引用。
//
// 问题 1（iter_move）：
//   std::ranges::iter_move(it) 默认行为是 std::move(*it)
//   对 tuple<T1&, T2&> 取右值引用得到 tuple<T1&, T2&>&&
//   但 tuple 内的 T1&/T2& 不会因此变成 T1&&/T2&&——元素级 move 不发生。
//   必须定制 iter_move 显式返回 tuple<T1&&, T2&&>。
//
// 问题 2（iter_swap）：
//   std::ranges::iter_swap(a, b) 默认行为是 std::ranges::swap(*a, *b)
//   swap(tuple<T1&,T2&>, tuple<T1&,T2&>) 只交换代理对象，引用不可重绑，语义错误。
//   必须定制 iter_swap，分别 swap 每一维的底层元素。
//
// 两者必须是 hidden friend（ADL 可找到），不能是普通成员函数。
// std::ranges::iter_move / iter_swap CPO 不走成员函数查找路径。

template<class It1, class It2>
struct ZipIterator {
    It1 it1_;
    It2 it2_;

    using value_type      = std::tuple<std::iter_value_t<It1>,
                                       std::iter_value_t<It2>>;
    using difference_type = std::common_type_t<std::iter_difference_t<It1>,
                                               std::iter_difference_t<It2>>;
    using reference       = std::tuple<std::iter_reference_t<It1>,
                                       std::iter_reference_t<It2>>;
    using iterator_concept  = std::random_access_iterator_tag;
    using iterator_category = std::input_iterator_tag; // proxy reference → input

    // TODO [必做] 5: 实现 operator*()
    //   return reference(*it1_, *it2_);
    reference operator*() const {
        // TODO [必做] 5:
        return reference(*it1_, *it2_);
    }

    ZipIterator& operator++()  { ++it1_; ++it2_; return *this; }
    ZipIterator  operator++(int) { auto t = *this; ++*this; return t; }
    ZipIterator& operator--()  { --it1_; --it2_; return *this; }
    ZipIterator  operator--(int) { auto t = *this; --*this; return t; }

    ZipIterator& operator+=(difference_type n) { it1_ += n; it2_ += n; return *this; }
    ZipIterator& operator-=(difference_type n) { it1_ -= n; it2_ -= n; return *this; }
    ZipIterator  operator+(difference_type n) const { auto t = *this; t += n; return t; }
    ZipIterator  operator-(difference_type n) const { auto t = *this; t -= n; return t; }
    difference_type operator-(const ZipIterator& o) const { return it1_ - o.it1_; }
    reference operator[](difference_type n) const { return *(*this + n); }

    auto operator<=>(const ZipIterator&) const = default;
    bool operator==(const ZipIterator&) const = default;

    // TODO [必做] 6: 实现 iter_move hidden friend
    //   返回 tuple<iter_rvalue_reference_t<It1>, iter_rvalue_reference_t<It2>>
    //   {ranges::iter_move(it.it1_), ranges::iter_move(it.it2_)}
    friend auto iter_move(const ZipIterator& it) {
        // TODO [必做] 6:
        return std::tuple<std::iter_rvalue_reference_t<It1>,
                          std::iter_rvalue_reference_t<It2>>(
            std::ranges::iter_move(it.it1_),
            std::ranges::iter_move(it.it2_)
        );
    }

    // TODO [必做] 7: 实现 iter_swap hidden friend
    //   分别对 it1_/it2_ 维调用 ranges::iter_swap
    friend void iter_swap(const ZipIterator& a, const ZipIterator& b) {
        // TODO [必做] 7:
        std::ranges::iter_swap(a.it1_, b.it1_);
        std::ranges::iter_swap(a.it2_, b.it2_);
    }
};

// ── static_assert 验证区 ──────────────────────────────────────────────────────

// iter_move 返回类型验证（proxy reference 必须返回 tuple<T&&, T&&>，不能是 tuple<T&, T&>）
using ZipIt = ZipIterator<std::vector<int>::iterator, std::vector<double>::iterator>;

static_assert(std::same_as<
    decltype(std::ranges::iter_move(std::declval<ZipIt>())),
    std::tuple<int&&, double&&>>,
    "iter_move 必须返回 tuple<T1&&, T2&&>，不能是 tuple<T1&, T2&>");

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    // 骨架阶段：仅验证编译
    // 实现 TODO 后取消注释以下所有测试

    /*
    // Part A: common_iterator 对接 C++17 算法
    {
        auto r = std::views::iota(1) | std::views::take(10);
        static_assert(!std::ranges::common_range<decltype(r)>);

        // views::common 包装后可传给 std::accumulate
        auto cr = r | std::views::common;
        static_assert(std::ranges::common_range<decltype(cr)>);
        int sum = std::accumulate(cr.begin(), cr.end(), 0);
        std::cout << "sum(1..10) = " << sum << '\n';
        assert(sum == 55);
    }

    // Part B: 手写 my_common_iterator 对接 std::accumulate
    {
        auto r = std::views::iota(1) | std::views::take(10);
        auto b = my_common_iterator<
            std::ranges::iterator_t<decltype(r)>,
            std::ranges::sentinel_t<decltype(r)>>{r.begin()};
        auto e = my_common_iterator<
            std::ranges::iterator_t<decltype(r)>,
            std::ranges::sentinel_t<decltype(r)>>{r.end()};
        int sum = std::accumulate(b, e, 0);
        assert(sum == 55);
    }

    // Part C: ZipIterator proxy sort
    {
        std::vector<int>    keys   = {3, 1, 4, 1, 5, 9, 2, 6};
        std::vector<double> values = {3.3, 1.1, 4.4, 1.2, 5.5, 9.9, 2.2, 6.6};

        ZipIterator<std::vector<int>::iterator,
                    std::vector<double>::iterator> begin{keys.begin(), values.begin()};
        ZipIterator<std::vector<int>::iterator,
                    std::vector<double>::iterator> end  {keys.end(),   values.end()};

        // ranges::sort 依赖 iter_move + iter_swap（通过 sortable concept 约束）
        std::ranges::sort(begin, end, {}, [](auto&& p){ return std::get<0>(p); });

        std::cout << "sorted by key:\n";
        for (auto i = begin; i != end; ++i) {
            auto [k, v] = *i;
            std::cout << "  " << k << " -> " << v << '\n';
        }
    }

    // iter_move 类型验证
    {
        std::vector<int>    keys   = {1};
        std::vector<double> values = {1.0};
        ZipIterator<std::vector<int>::iterator,
                    std::vector<double>::iterator> it{keys.begin(), values.begin()};
        auto moved = std::ranges::iter_move(it);
        static_assert(std::same_as<decltype(moved), std::tuple<int&&, double&&>>);
        std::cout << "iter_move 类型验证通过\n";
    }

    std::cout << "H-2 all assertions passed.\n";
    */

    std::cout << "H-2 skeleton compiled.\n";
    return 0;
}
