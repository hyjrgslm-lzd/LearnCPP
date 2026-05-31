#pragma once
// =============================================================================
// my_ranges/04_factories.hpp — 层 4：工厂 view（iota_view / single_view）骨架
//
// 对应章节：模块 F/G — 完成型 view（无底层依赖，自持数据）
//
// 关键特性：
//   iota_view：整数序列，iterator_concept = random_access；enable_borrowed_range = true
//   single_view：持有单个值，begin/end = 裸指针，iterator_concept 自动为 contiguous
//
// TODO 列表：
//   [必做] 4a. 补全 iota_view::iterator 的 operator-- 和 operator+= / operator-=
//             （使其真正满足 random_access_iterator concept）
//   [必做] 4b. 补全 iota_view::iterator 的 operator[] 和 operator+ / operator-
//   [必做] 4c. 补全 single_view 的 const 版本 begin/end（const T* 重载）
//   [进阶] 4d. 给 iota_view 增加无界版本（W b 只有下界，迭代器永不到达 end）
//   [进阶] 4e. 给 single_view 使用 movable-box 包装（让不可默认构造的 T 也能工作）
// =============================================================================

#include "03_interface.hpp"
#include <concepts>
#include <memory>    // std::addressof

namespace my::views {

// ---------------------------------------------------------------------------
// iota_view：有界整数序列
// ---------------------------------------------------------------------------
template<std::integral W>
class iota_view : public my::ranges::view_interface<iota_view<W>> {
    W _b, _e;
public:
    iota_view(W b, W e) : _b(b), _e(e) {}

    struct iterator {
        using value_type       = W;
        using difference_type  = std::ptrdiff_t;
        using iterator_concept = std::random_access_iterator_tag;

        W _cur;

        // 解引用
        W operator*() const { return _cur; }

        // 前进
        iterator& operator++() { ++_cur; return *this; }
        iterator  operator++(int) { auto tmp = *this; ++*this; return tmp; }

        // TODO [必做] 4a：补全 operator--（后退一步）
        // iterator& operator--() { --_cur; return *this; }
        // iterator  operator--(int) { auto tmp = *this; --*this; return tmp; }

        // TODO [必做] 4b：补全 operator+=, operator-=, operator+, operator-, operator[]
        // iterator& operator+=(difference_type n) { _cur += static_cast<W>(n); return *this; }
        // iterator& operator-=(difference_type n) { _cur -= static_cast<W>(n); return *this; }
        // friend iterator operator+(iterator i, difference_type n) { i += n; return i; }
        // friend iterator operator+(difference_type n, iterator i) { i += n; return i; }
        // friend iterator operator-(iterator i, difference_type n) { i -= n; return i; }
        // W operator[](difference_type n) const { return _cur + static_cast<W>(n); }

        // 距离（random_access 要求）
        friend difference_type operator-(iterator a, iterator b) {
            return static_cast<difference_type>(a._cur) - static_cast<difference_type>(b._cur);
        }

        // 相等比较
        bool operator==(const iterator&) const = default;

        // TODO [必做] 4b：补全三路比较（<=>）
        // auto operator<=>(const iterator&) const = default;
    };

    iterator begin() const { return {_b}; }
    iterator end()   const { return {_e}; }
};

// 工厂 lambda（closure 层使用）
constexpr auto iota = [](auto b, auto e) {
    return iota_view{b, e};
};

} // namespace my::views

// enable_view / enable_borrowed_range 特化 —— 必须在 primary template 所在命名空间（my::ranges）
namespace my::ranges {
    template<std::integral W>
    inline constexpr bool enable_view<my::views::iota_view<W>> = true;

    template<std::integral W>
    inline constexpr bool enable_borrowed_range<my::views::iota_view<W>> = true;
} // namespace my::ranges

namespace my::views {

// ---------------------------------------------------------------------------
// single_view：持有单个值
// ---------------------------------------------------------------------------
template<std::copy_constructible T>
class single_view : public my::ranges::view_interface<single_view<T>> {
    T _v;
public:
    explicit single_view(T v) : _v(std::move(v)) {}

    // begin/end 返回裸指针 → iterator_concept 自动为 contiguous
    T* begin() noexcept { return std::addressof(_v); }
    T* end()   noexcept { return begin() + 1; }

    // TODO [必做] 4c：补全 const 重载
    // const T* begin() const noexcept { return std::addressof(_v); }
    // const T* end()   const noexcept { return begin() + 1; }

    // size() 始终为 1（不依赖 view_interface 注入）
    static constexpr std::size_t size() noexcept { return 1; }
};

constexpr auto single = [](auto v) {
    return single_view{std::move(v)};
};

} // namespace my::views

namespace my::ranges {
    template<typename T>
    inline constexpr bool enable_view<my::views::single_view<T>> = true;
} // namespace my::ranges
