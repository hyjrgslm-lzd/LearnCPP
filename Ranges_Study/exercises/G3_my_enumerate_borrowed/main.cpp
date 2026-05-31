// 章节：模块 G — 自行实现视图
// 小节：G-3 my_enumerate_view + enable_borrowed_range 条件特化
// 提案：P0896R4（ranges 核心），P1739R4（borrowed_range），P2164（C++26 views::enumerate 设计参考）
// C++ 标准：C++26
//
// 预期输出（实现完成后）：
//   基础迭代通过
//   管道语法通过
//   与 transform 组合通过
//   enable_borrowed_range 验证通过
//   view concept 验证通过
//   G-3 all assertions passed.
//
// 默认输出（骨架阶段）：
//   G-3 skeleton compiled.

#include <ranges>
#include <iterator>
#include <concepts>
#include <utility>
#include <vector>
#include <list>
#include <string_view>
#include <cassert>
#include <iostream>

// ── my_enumerate_view<V> ──────────────────────────────────────────────────────
//
// 内部迭代器 iterator 结构：
//   - InnerIt it_：底层迭代器
//   - size_t idx_：当前索引（迭代器内部状态，不依赖 view 对象）
//   - operator*() 返回 pair<size_t, range_reference_t<V>>（proxy pair，prvalue）
//   - reference = pair<size_t, range_reference_t<V>>
//   - value_type  = pair<size_t, range_value_t<V>>
//   - iterator_concept：继承底层，上界 random_access_iterator_tag
//   - iterator_category：proxy pair 是 prvalue → input_iterator_tag
//     （C++17 算法要求 *it 是可赋值左值，pair<size_t, T&> 不满足）
//   - iter_move hidden friend：返回 pair<size_t, range_rvalue_reference_t<V>>
//     （proxy reference 必须定制 iter_move，否则 move 语义不正确）
//
// enable_borrowed_range 条件特化（在类定义之后）：
//   template<view V>
//   inline constexpr bool enable_borrowed_range<my_enumerate_view<V>> =
//       enable_borrowed_range<V>;
//   语义：若底层 V 是 borrowed_range（迭代器不依赖 V 生命周期），则 my_enumerate_view<V> 也是。
//   idx_ 是迭代器内部值拷贝，不依赖 view 对象——因此借用属性可以透传。

template <std::ranges::view V>
class my_enumerate_view
    : public std::ranges::view_interface<my_enumerate_view<V>> {

    // ── 内部迭代器 ────────────────────────────────────────────────────────────
    struct iterator {
    private:
        using InnerIt = std::ranges::iterator_t<V>;
        InnerIt    it_{};
        std::size_t idx_{};

    public:
        using reference     = std::pair<std::size_t, std::ranges::range_reference_t<V>>;
        using value_type    = std::pair<std::size_t, std::ranges::range_value_t<V>>;
        using difference_type = std::ranges::range_difference_t<V>;

        // iterator_concept：继承底层，上界 random_access
        using iterator_concept =
            std::conditional_t<std::random_access_iterator<InnerIt>,
                std::random_access_iterator_tag,
            std::conditional_t<std::bidirectional_iterator<InnerIt>,
                std::bidirectional_iterator_tag,
            std::conditional_t<std::forward_iterator<InnerIt>,
                std::forward_iterator_tag,
                std::input_iterator_tag>>>;

        // iterator_category：proxy pair 是 prvalue → input_iterator_tag
        using iterator_category = std::input_iterator_tag;

        iterator() = default;
        constexpr iterator(InnerIt it, std::size_t idx) : it_(it), idx_(idx) {}

        // TODO [必做] 1: 实现 operator*()
        //   返回 reference{idx_, *it_}
        constexpr reference operator*() const {
            // TODO [必做] 1:
            return reference{idx_, *it_};
        }

        // TODO [必做] 2: 实现 operator++()（前置/后置）
        //   前置：++it_; ++idx_; return *this;
        constexpr iterator& operator++() {
            // TODO [必做] 2:
            ++it_; ++idx_; return *this;
        }
        constexpr iterator operator++(int) {
            auto tmp = *this; ++(*this); return tmp;
        }

        // TODO [必做] 3: 实现 bidirectional operator--()（requires bidirectional_iterator<InnerIt>）
        //   --it_; --idx_; return *this;
        constexpr iterator& operator--()
            requires std::bidirectional_iterator<InnerIt>
        {
            // TODO [必做] 3:
            --it_; --idx_; return *this;
        }
        constexpr iterator operator--(int)
            requires std::bidirectional_iterator<InnerIt>
        { auto tmp = *this; --(*this); return tmp; }

        // TODO [进阶] A: 实现 random_access 运算符
        //   +=, -=, +, -, [], <=>（均 requires random_access_iterator<InnerIt>）
        //   注意 idx_ 是 size_t，n 是有符号 difference_type，需要 static_cast
        constexpr iterator& operator+=(difference_type n)
            requires std::random_access_iterator<InnerIt>
        {
            // TODO [进阶] A:
            it_ += n; idx_ += static_cast<std::size_t>(n); return *this;
        }
        constexpr iterator& operator-=(difference_type n)
            requires std::random_access_iterator<InnerIt>
        { it_ -= n; idx_ -= static_cast<std::size_t>(n); return *this; }

        constexpr iterator operator+(difference_type n) const
            requires std::random_access_iterator<InnerIt>
        { return iterator{it_ + n, idx_ + static_cast<std::size_t>(n)}; }

        friend constexpr iterator operator+(difference_type n, const iterator& it)
            requires std::random_access_iterator<InnerIt>
        { return it + n; }

        constexpr iterator operator-(difference_type n) const
            requires std::random_access_iterator<InnerIt>
        { return iterator{it_ - n, idx_ - static_cast<std::size_t>(n)}; }

        constexpr difference_type operator-(const iterator& other) const
            requires std::random_access_iterator<InnerIt>
        { return it_ - other.it_; }

        constexpr reference operator[](difference_type n) const
            requires std::random_access_iterator<InnerIt>
        { return reference{idx_ + static_cast<std::size_t>(n), it_[n]}; }

        constexpr bool operator==(const iterator& other) const
            requires std::equality_comparable<InnerIt>
        { return it_ == other.it_; }

        constexpr auto operator<=>(const iterator& other) const
            requires std::random_access_iterator<InnerIt>
        { return it_ <=> other.it_; }

        // TODO [必做] 4: 实现 iter_move hidden friend
        //   返回 pair<size_t, range_rvalue_reference_t<V>>{it.idx_, ranges::iter_move(it.it_)}
        //   必须是 hidden friend（ADL 可找到），不能是普通成员函数
        friend constexpr auto iter_move(const iterator& it)
            noexcept(noexcept(std::ranges::iter_move(it.it_)))
        {
            // TODO [必做] 4:
            return std::pair<std::size_t,
                             std::ranges::range_rvalue_reference_t<V>>{
                it.idx_,
                std::ranges::iter_move(it.it_)
            };
        }
    };

public:
    my_enumerate_view() = default;

    explicit constexpr my_enumerate_view(V base)
        : base_(std::move(base)) {}

    // TODO [必做] 5: 实现 begin()
    //   返回 iterator{ranges::begin(base_), 0}
    constexpr iterator begin() {
        // TODO [必做] 5:
        return iterator{std::ranges::begin(base_), 0};
    }

    // TODO [必做] 6: 实现 end()（common_range<V> 和非 common_range<V> 两个重载）
    //   common_range 分支：需要 end-index，用 ranges::distance(base_)（O(1) 当底层是 sized_range）
    //   非 common_range 分支：直接返回 ranges::end(base_)
    //   注意：若底层不是 sized_range，distance 变成 O(n)——要在 requires 里加 sized_range 约束避免
    constexpr iterator end()
        requires std::ranges::common_range<V>
    {
        // TODO [必做] 6a:
        return iterator{std::ranges::end(base_),
                        static_cast<std::size_t>(std::ranges::distance(base_))};
    }

    constexpr auto end()
        requires (!std::ranges::common_range<V>)
    {
        // TODO [必做] 6b:
        return std::ranges::end(base_);
    }

    constexpr auto size()
        requires std::ranges::sized_range<V>
    { return std::ranges::size(base_); }

    constexpr V base() const& { return base_; }
    constexpr V base() &&     { return std::move(base_); }

private:
    V base_{};
};

// ── 推导指引 ──────────────────────────────────────────────────────────────────
template <std::ranges::viewable_range R>
my_enumerate_view(R&&) -> my_enumerate_view<std::views::all_t<R>>;

// ── enable_borrowed_range 条件特化 ────────────────────────────────────────────
//
// TODO [必做] 7: 实现条件特化
//   template<view V>
//   inline constexpr bool std::ranges::enable_borrowed_range<my_enumerate_view<V>> =
//       std::ranges::enable_borrowed_range<V>;
//
// 语义：my_enumerate_view 的迭代器持有底层 InnerIt 和 idx_ 值拷贝。
//   若 V 是 borrowed_range，InnerIt 在 V 销毁后仍有效；
//   idx_ 是迭代器内部值，不依赖 view 对象——因此可以透传 V 的 borrowed 属性。
//
// 特化必须在 my_enumerate_view 定义之后、main 使用之前出现。

template <std::ranges::view V>
inline constexpr bool
    std::ranges::enable_borrowed_range<my_enumerate_view<V>> =
        std::ranges::enable_borrowed_range<V>;

// ── my_enumerate_closure + my_enumerate_fn ────────────────────────────────────
//
// my_enumerate_closure：无参数 closure，继承 range_adaptor_closure
//   operator()(R&&)：返回 my_enumerate_view{views::all(forward<R>(r))}
//
// my_enumerate_fn：
//   operator()()           → my_enumerate_closure
//   operator()(viewable_range R) → my_enumerate_view 直接构造

class my_enumerate_closure
    : public std::ranges::range_adaptor_closure<my_enumerate_closure> {
public:
    // TODO [必做] 8: 实现 operator()(R&& r)
    template <std::ranges::viewable_range R>
    constexpr auto operator()(R&& r) const {
        // TODO [必做] 8:
        return my_enumerate_view{std::views::all(std::forward<R>(r))};
    }
};

struct my_enumerate_fn {
    // 无参数 → 返回 closure
    constexpr my_enumerate_closure operator()() const {
        return my_enumerate_closure{};
    }

    // 直接传入 range → 立即构造 view
    template <std::ranges::viewable_range R>
    constexpr auto operator()(R&& r) const {
        return my_enumerate_view{std::views::all(std::forward<R>(r))};
    }
};

inline constexpr my_enumerate_fn my_enumerate{};

// ── static_assert 验证区 ──────────────────────────────────────────────────────

// 验证 enable_borrowed_range 条件特化（在特化生效后）
// 骨架阶段：TODO 尚未编写条件特化；实现后取消注释这两条断言。
// static_assert(std::ranges::borrowed_range<my_enumerate_view<std::string_view>>,
//               "string_view 底层 → my_enumerate_view 也是 borrowed_range");
//
// static_assert(!std::ranges::borrowed_range<
//     my_enumerate_view<std::views::all_t<std::vector<int>&>>>,
//     "vector 底层 → my_enumerate_view 不是 borrowed_range");

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    // 骨架阶段：仅验证编译
    // 实现 TODO 后取消注释以下所有测试

    /*
    std::vector<int> vec{10, 20, 30, 40, 50};

    // 基础迭代：验证 (index, value) pair
    {
        std::size_t expected_idx[] = {0, 1, 2, 3, 4};
        int         expected_val[] = {10, 20, 30, 40, 50};
        int i = 0;
        for (auto [idx, val] : my_enumerate(vec)) {
            assert(idx == expected_idx[i]);
            assert(val == expected_val[i]);
            ++i;
        }
        assert(i == 5);
        std::cout << "基础迭代通过\n";
    }

    // 管道语法：vec | my_enumerate()
    {
        int i = 0;
        for (auto [idx, val] : vec | my_enumerate()) {
            assert(idx == static_cast<std::size_t>(i));
            assert(val == vec[i]);
            ++i;
        }
        std::cout << "管道语法通过\n";
    }

    // 与 views::transform 组合
    {
        auto extract_value = [](auto p) { return p.second; };
        auto combined = vec | my_enumerate() | std::views::transform(extract_value);
        int i = 0;
        for (int v : combined) assert(v == vec[i++]);
        assert(i == 5);
        std::cout << "与 transform 组合通过\n";
    }

    // enable_borrowed_range 验证
    {
        std::string_view sv{"hello"};
        using SVEnumView = decltype(my_enumerate(sv));
        static_assert(std::ranges::borrowed_range<SVEnumView>);

        using VecEnumView = decltype(my_enumerate(vec));
        static_assert(!std::ranges::borrowed_range<VecEnumView>);

        std::cout << "enable_borrowed_range 验证通过\n";
    }

    // view concept + iterator_concept/iterator_category 验证
    {
        auto ev = my_enumerate(vec);
        static_assert(std::ranges::view<decltype(ev)>);
        static_assert(std::ranges::input_range<decltype(ev)>);
        static_assert(std::same_as<
            typename decltype(ev)::iterator::iterator_concept,
            std::random_access_iterator_tag>);
        static_assert(std::same_as<
            typename decltype(ev)::iterator::iterator_category,
            std::input_iterator_tag>);
        std::cout << "view concept 验证通过\n";
    }

    std::cout << "G-3 all assertions passed.\n";
    */

    std::cout << "G-3 skeleton compiled.\n";
    return 0;
}
