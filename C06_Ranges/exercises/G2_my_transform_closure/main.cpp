// 章节：模块 G — 自行实现视图
// 小节：G-2 my_transform_view + range_adaptor_closure 注入 operator|（P2387R3）
// 提案：P2387R3（range_adaptor_closure CRTP 正式化），P0896R4（transform_view 原始设计）
// C++ 标准：C++26
//
// 预期输出（实现完成后）：
//   用法 1 通过
//   用法 2 通过
//   用法 3 通过
//   view concept 验证通过
//   G-2 all assertions passed.
//
// 默认输出（骨架阶段）：
//   G-2 skeleton compiled.

#include <ranges>
#include <iterator>
#include <concepts>
#include <functional>
#include <vector>
#include <list>
#include <cassert>
#include <iostream>

// ── my_transform_view<V, F> ───────────────────────────────────────────────────
//
// 内部迭代器 iterator 结构：
//   - F* f_：指向父 view 存储的 F（非 owning，避免在迭代器间重复拷贝函数对象）
//   - InnerIt it_：底层迭代器
//   - operator*() 返回 std::invoke(*f_, *it_)
//   - iterator_concept：继承底层，上界 random_access_iterator_tag
//   - iterator_category：invoke 结果通常是 prvalue，C++17 约定最高 forward；
//     若 reference 是真实左值引用则继承底层
//   - reference = invoke_result_t<F&, range_reference_t<V>>
//   - value_type = remove_cvref_t<reference>
//
// my_transform_view 成员：
//   - begin()：返回 iterator{f_, ranges::begin(base_)}
//   - end()（common_range<V>）：返回 iterator{f_, ranges::end(base_)}
//   - end()（非 common_range<V>）：返回 ranges::end(base_)
//   - size()（requires sized_range<V>）：返回 ranges::size(base_)

template <std::ranges::view V, std::copy_constructible F>
    requires std::is_object_v<F> &&
             std::regular_invocable<F&, std::ranges::range_reference_t<V>>
class my_transform_view
    : public std::ranges::view_interface<my_transform_view<V, F>> {

    // ── 内部迭代器 ────────────────────────────────────────────────────────────
    struct iterator {
    private:
        using InnerIt = std::ranges::iterator_t<V>;
        F*       f_;
        InnerIt  it_;

    public:
        using reference     = std::invoke_result_t<F&, std::ranges::range_reference_t<V>>;
        using value_type    = std::remove_cvref_t<reference>;
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

        // iterator_category：invoke 结果通常是 prvalue，保守设为 forward（当底层 forward 时）
        using iterator_category =
            std::conditional_t<
                std::is_lvalue_reference_v<reference> &&
                    std::derived_from<
                        typename std::iterator_traits<InnerIt>::iterator_category,
                        std::forward_iterator_tag>,
                typename std::iterator_traits<InnerIt>::iterator_category,
                std::conditional_t<std::forward_iterator<InnerIt>,
                    std::forward_iterator_tag,
                    std::input_iterator_tag>>;

        iterator() = default;
        constexpr iterator(F& f, InnerIt it) : f_(&f), it_(it) {}

        // TODO [必做] 1: 实现 operator*()
        //   return std::invoke(*f_, *it_);
        constexpr reference operator*() const {
            // TODO [必做] 1:
            return std::invoke(*f_, *it_);
        }

        // TODO [必做] 2: 实现 operator++() 前置/后置
        constexpr iterator& operator++() {
            // TODO [必做] 2: ++it_; return *this;
            ++it_; return *this;
        }
        constexpr iterator operator++(int) {
            auto tmp = *this; ++(*this); return tmp;
        }

        // TODO [必做] 3: 实现 bidirectional operator--()（requires bidirectional_iterator<InnerIt>）
        constexpr iterator& operator--()
            requires std::bidirectional_iterator<InnerIt>
        {
            // TODO [必做] 3: --it_; return *this;
            --it_; return *this;
        }
        constexpr iterator operator--(int)
            requires std::bidirectional_iterator<InnerIt>
        { auto tmp = *this; --(*this); return tmp; }

        // TODO [进阶] A: 实现 random_access 运算符（+=, -=, +, -, [], <=>）
        //   均 requires random_access_iterator<InnerIt>
        constexpr iterator& operator+=(difference_type n)
            requires std::random_access_iterator<InnerIt>
        { it_ += n; return *this; }

        constexpr iterator& operator-=(difference_type n)
            requires std::random_access_iterator<InnerIt>
        { it_ -= n; return *this; }

        constexpr iterator operator+(difference_type n) const
            requires std::random_access_iterator<InnerIt>
        { return iterator{*f_, it_ + n}; }

        friend constexpr iterator operator+(difference_type n, const iterator& it)
            requires std::random_access_iterator<InnerIt>
        { return it + n; }

        constexpr iterator operator-(difference_type n) const
            requires std::random_access_iterator<InnerIt>
        { return iterator{*f_, it_ - n}; }

        constexpr difference_type operator-(const iterator& other) const
            requires std::random_access_iterator<InnerIt>
        { return it_ - other.it_; }

        constexpr reference operator[](difference_type n) const
            requires std::random_access_iterator<InnerIt>
        { return std::invoke(*f_, it_[n]); }

        constexpr bool operator==(const iterator& other) const
            requires std::equality_comparable<InnerIt>
        { return it_ == other.it_; }

        constexpr auto operator<=>(const iterator& other) const
            requires std::random_access_iterator<InnerIt>
        { return it_ <=> other.it_; }
    };

public:
    my_transform_view() = default;

    constexpr my_transform_view(V base, F f)
        : base_(std::move(base)), f_(std::move(f)) {}

    // TODO [必做] 4: 实现 begin()
    //   返回 iterator{f_, ranges::begin(base_)}
    constexpr iterator begin() {
        // TODO [必做] 4:
        return iterator{f_, std::ranges::begin(base_)};
    }

    // TODO [必做] 5: 实现 end()（common_range<V> 和非 common_range<V> 两个重载）
    //   common_range 分支：返回 iterator{f_, ranges::end(base_)}
    //   非 common_range 分支：返回 ranges::end(base_)
    constexpr iterator end()
        requires std::ranges::common_range<V>
    {
        // TODO [必做] 5a:
        return iterator{f_, std::ranges::end(base_)};
    }

    constexpr auto end()
        requires (!std::ranges::common_range<V>)
    {
        // TODO [必做] 5b:
        return std::ranges::end(base_);
    }

    constexpr auto size()
        requires std::ranges::sized_range<V>
    { return std::ranges::size(base_); }

    constexpr V base() const& { return base_; }
    constexpr V base() &&     { return std::move(base_); }

private:
    V base_{};
    F f_{};
};

// ── 推导指引 ──────────────────────────────────────────────────────────────────
template <std::ranges::viewable_range R, typename F>
my_transform_view(R&&, F) -> my_transform_view<std::views::all_t<R>, F>;

// ── my_transform_closure<F>：range_adaptor_closure CRTP ───────────────────────
//
// 继承 range_adaptor_closure<my_transform_closure<F>> 自动获得：
//   1. operator|(range, closure)  → closure(range)
//   2. operator|(closure, closure) → 新 closure
//
// operator()：接受 viewable_range，返回 my_transform_view
//   template<viewable_range R> requires regular_invocable<F&, range_reference_t<R>>
//   constexpr auto operator()(R&& r) const { ... }
//
// 注意：closure 必须值持有 F（不能存引用），因为 closure 可能比原始函数对象活得长。

template <std::copy_constructible F>
class my_transform_closure
    : public std::ranges::range_adaptor_closure<my_transform_closure<F>> {
public:
    explicit constexpr my_transform_closure(F f) : f_(std::move(f)) {}

    // TODO [必做] 6: 实现 operator()(R&& r)
    //   返回 my_transform_view{views::all(forward<R>(r)), f_}
    template <std::ranges::viewable_range R>
        requires std::regular_invocable<F&, std::ranges::range_reference_t<R>>
    constexpr auto operator()(R&& r) const {
        // TODO [必做] 6:
        return my_transform_view{std::views::all(std::forward<R>(r)), f_};
    }

private:
    F f_;
};

// ── my_transform_fn：工厂对象 ──────────────────────────────────────────────────
//
// 两种调用形式：
//   my_transform(f)         → my_transform_closure<F>
//   my_transform(range, f)  → my_transform_view 直接构造
//
// TODO [必做] 7: 实现 my_transform_fn 的两个 operator() 重载

struct my_transform_fn {
    // 单参数：返回 closure
    template <std::copy_constructible F>
    constexpr auto operator()(F f) const {
        // TODO [必做] 7a:
        return my_transform_closure<F>{std::move(f)};
    }

    // 双参数：直接构造 view
    template <std::ranges::viewable_range R, std::copy_constructible F>
        requires std::regular_invocable<F&, std::ranges::range_reference_t<R>>
    constexpr auto operator()(R&& r, F f) const {
        // TODO [必做] 7b:
        return my_transform_view{std::views::all(std::forward<R>(r)), std::move(f)};
    }
};

inline constexpr my_transform_fn my_transform{};

// ── static_assert 验证区 ──────────────────────────────────────────────────────

// my_transform_closure 满足 range_adaptor_closure（继承后自动具备 operator|）
// 骨架阶段：TODO 7 尚未补全 CRTP 继承；实现后取消注释。
// static_assert(std::derived_from<
//     my_transform_closure<decltype([](int x){ return x; })>,
//     std::ranges::range_adaptor_closure<my_transform_closure<decltype([](int x){ return x; })>>>);

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    // 骨架阶段：仅验证编译
    // 实现 TODO 后取消注释以下所有测试

    /*
    std::vector<int> vec{1, 2, 3, 4, 5};
    auto sq  = [](int x){ return x * x; };
    auto inc = [](int x){ return x + 1; };

    // 用法 1：vec | my_transform(sq)
    {
        auto r = vec | my_transform(sq);
        int expected[] = {1, 4, 9, 16, 25};
        int i = 0;
        for (int x : r) assert(x == expected[i++]);
        assert(i == 5);
        std::cout << "用法 1 通过\n";
    }

    // 用法 2：链式管道
    {
        auto r = vec | my_transform(sq) | my_transform(inc);
        int expected[] = {2, 5, 10, 17, 26};
        int i = 0;
        for (int x : r) assert(x == expected[i++]);
        assert(i == 5);
        std::cout << "用法 2 通过\n";
    }

    // 用法 3：closure-to-closure 组合（P2387R3 保证 closure | closure 结果仍是 closure）
    {
        auto combined = my_transform(sq) | my_transform(inc);
        auto r = vec | combined;
        int expected[] = {2, 5, 10, 17, 26};
        int i = 0;
        for (int x : r) assert(x == expected[i++]);
        assert(i == 5);
        std::cout << "用法 3 通过\n";
    }

    // view concept 验证
    {
        auto r = my_transform(vec, sq);
        static_assert(std::ranges::view<decltype(r)>);
        static_assert(std::ranges::random_access_range<decltype(r)>);
        std::cout << "view concept 验证通过\n";
    }

    std::cout << "G-2 all assertions passed.\n";
    */

    std::cout << "G-2 skeleton compiled.\n";
    return 0;
}
