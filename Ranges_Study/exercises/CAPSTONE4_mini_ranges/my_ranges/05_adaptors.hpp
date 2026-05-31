#pragma once
// =============================================================================
// my_ranges/05_adaptors.hpp — 层 5：adaptor view + closure 骨架
//
// 对应章节：模块 G — inner iterator，range_adaptor_closure，transform / take
//
// 关键特性：
//   transform_view：inner iterator 持有底层迭代器 + parent 指针；
//                   contiguous 底层降为 random_access（operator* 返回 prvalue）
//   take_view：sentinel 异型（end() 类型 ≠ begin() 类型）；
//              本实现仅对 random_access 底层有效
//   _transform_closure / _take_closure：继承 range_adaptor_closure，注入 operator|
//
// TODO 列表：
//   [必做] 5a. 补全 transform_view::iterator 的 operator++ 后置版本 + operator==
//   [必做] 5b. 补全 transform_view::iterator 的 random_access 操作
//             （operator+= / operator- / operator[]，需先完成 04 的 iota_view 相关操作）
//   [必做] 5c. 补全 take_view::sentinel 的 operator==（iterator 侧重载）
//   [必做] 5d. 验证 take_view::begin() 和 end() 的返回类型不同（sentinel 异型）
//   [进阶] 5e. 增加 iter_move hidden-friend 到 transform_view::iterator
//   [进阶] 5f. 实现 filter_view（需要 __non_propagating_cache + begin() 非 const）
// =============================================================================

#include "04_factories.hpp"
#include <functional>  // std::invoke
#include <type_traits>

namespace my::views {

// ---------------------------------------------------------------------------
// transform_view
// ---------------------------------------------------------------------------
template<my::ranges::range V, std::copy_constructible F>
    requires my::ranges::view<V>
class transform_view : public my::ranges::view_interface<transform_view<V, F>> {
    V _base;
    F _fun;
public:
    transform_view(V b, F f) : _base(std::move(b)), _fun(std::move(f)) {}

    struct iterator {
        using _BaseIt = my::ranges::iterator_t<V>;

        // iterator_concept：contiguous 底层降为 random_access（operator* 返回 prvalue）
        using iterator_concept = std::conditional_t<
            std::contiguous_iterator<_BaseIt>,
            std::random_access_iterator_tag,
            typename std::iterator_traits<_BaseIt>::iterator_category
        >;
        using value_type      = std::remove_cvref_t<
            std::invoke_result_t<F&, my::ranges::range_reference_t<V>>>;
        using difference_type = std::iter_difference_t<_BaseIt>;

        _BaseIt _cur;
        F*      _fp;   // points-to parent 的函数对象

        // 解引用：调用 invoke，支持成员指针/仿函数/函数指针
        decltype(auto) operator*() const { return std::invoke(*_fp, *_cur); }

        // 前进
        iterator& operator++() { ++_cur; return *this; }

        // TODO [必做] 5a：补全后置 operator++
        // iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }

        // 相等比较
        bool operator==(const iterator& o) const { return _cur == o._cur; }

        // TODO [必做] 5b：补全 random_access 操作（+=, -=, +, -, [], <=>）
        // iterator& operator+=(difference_type n) { _cur += n; return *this; }
        // iterator& operator-=(difference_type n) { _cur -= n; return *this; }
        // friend iterator operator+(iterator i, difference_type n) { i += n; return i; }
        // friend iterator operator+(difference_type n, iterator i) { i += n; return i; }
        // friend iterator operator-(iterator i, difference_type n) { i -= n; return i; }
        // friend difference_type operator-(const iterator& a, const iterator& b) { return a._cur - b._cur; }
        // decltype(auto) operator[](difference_type n) const { return std::invoke(*_fp, _cur[n]); }
        // auto operator<=>(const iterator&) const = default;

        // TODO [进阶] 5e：iter_move hidden-friend
        // friend decltype(auto) iter_move(const iterator& i) {
        //     return std::invoke(*i._fp, my::ranges::iter_move(i._cur));
        // }
    };

    iterator begin() { return {my::ranges::begin(_base), &_fun}; }
    iterator end()   { return {my::ranges::end(_base),   &_fun}; }
};

// enable_view 特化移到命名空间末尾（my::ranges 命名空间内），避免跨命名空间特化。
// transform_view 不设 enable_borrowed_range（迭代器持有 parent 指针，依赖 view 生命周期）

// ---------------------------------------------------------------------------
// take_view：sentinel 异型
// ---------------------------------------------------------------------------
template<my::ranges::range V>
    requires my::ranges::view<V>
class take_view : public my::ranges::view_interface<take_view<V>> {
    V                _base;
    std::ptrdiff_t   _n;
public:
    take_view(V b, std::ptrdiff_t n) : _base(std::move(b)), _n(n) {}

    // sentinel：类型与 iterator_t<V> 不同（异型）
    struct sentinel {
        my::ranges::iterator_t<V> _begin;
        std::ptrdiff_t            _n;

        // TODO [必做] 5c：operator== 判断迭代器是否到达第 n 个位置
        bool operator==(const my::ranges::iterator_t<V>& it) const
            requires std::random_access_iterator<my::ranges::iterator_t<V>>
        {
            // TODO [必做] 5c：实现：return it == _begin + _n;
            return it == _begin + _n;
        }

        // 对称重载（it == sentinel）
        friend bool operator==(const my::ranges::iterator_t<V>& it, const sentinel& s) {
            return s == it;
        }
    };

    // begin() 返回 iterator_t<V>；end() 返回 sentinel（两者类型不同）
    auto begin() { return my::ranges::begin(_base); }
    auto end()   {
        // TODO [必做] 5d：验证 begin() 和 end() 返回类型不同
        //   static_assert(!std::is_same_v<decltype(begin()), decltype(end())>);
        return sentinel{my::ranges::begin(_base), _n};
    }
};

// enable_view<take_view> 特化也移到 my::ranges 命名空间（见文件末尾）

// ---------------------------------------------------------------------------
// closure 类型
// ---------------------------------------------------------------------------

// transform closure
template<typename F>
struct _transform_closure
    : my::ranges::range_adaptor_closure<_transform_closure<F>>
{
    F _f;
    explicit _transform_closure(F f) : _f(std::move(f)) {}

    template<my::ranges::range R>
    auto operator()(R&& r) const {
        return transform_view{std::forward<R>(r), _f};
    }
};

// take closure
template<typename N>
struct _take_closure
    : my::ranges::range_adaptor_closure<_take_closure<N>>
{
    N _n;
    explicit _take_closure(N n) : _n(n) {}

    template<my::ranges::range R>
    auto operator()(R&& r) const {
        return take_view{std::forward<R>(r), static_cast<std::ptrdiff_t>(_n)};
    }
};

// 工厂 lambda（创建 closure 对象）
constexpr auto transform = [](auto f) {
    return _transform_closure{std::move(f)};
};

constexpr auto take = [](auto n) {
    return _take_closure{n};
};

// ---------------------------------------------------------------------------
// filter_view 骨架（进阶 5f）
// ---------------------------------------------------------------------------
// TODO [进阶] 5f：实现 filter_view
//   - 需要 __non_propagating_cache<std::optional<iterator_t<V>>> 成员 _M_begin
//   - begin() 是非 const 成员函数（写缓存）
//   - iterator_concept = min(底层, bidirectional)
//   - filter_view 不是 sized_range

} // namespace my::views

namespace my::ranges {
    template<typename V, typename F>
    inline constexpr bool enable_view<my::views::transform_view<V, F>> = true;

    template<typename V>
    inline constexpr bool enable_view<my::views::take_view<V>> = true;
} // namespace my::ranges
