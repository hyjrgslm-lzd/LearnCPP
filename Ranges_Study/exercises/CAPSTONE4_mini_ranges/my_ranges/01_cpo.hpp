#pragma once
// =============================================================================
// my_ranges/01_cpo.hpp — 层 1：CPO（定制点对象）骨架
//
// 对应章节：模块 E — CPO / niebloid，ADL 隔离
//
// 设计原则：
//   CPO 是 inline constexpr 函数对象，不是函数模板。
//   不参与 ADL 查找，阻止第三方同名函数意外劫持行为。
//   优先级链：成员函数优先 → ADL 次之 → 标准回退。
//
// TODO 列表：
//   [必做] 1a. 补全 begin CPO 的 ADL 回退路径（目前仅覆盖成员 begin()）
//   [必做] 1b. 补全 end CPO（对称实现）
//   [必做] 1c. 实现 iter_move CPO（hidden-friend 优先，回退 std::move(*it)）
//   [进阶] 1d. 增加 size CPO（成员 size() → ranges::end - ranges::begin 回退）
//   [进阶] 1e. 增加 data CPO（仅对 contiguous_range 有效）
// =============================================================================

#include <iterator>
#include <utility>
#include <type_traits>

namespace my::ranges {

// ---------------------------------------------------------------------------
// begin CPO
// ---------------------------------------------------------------------------
namespace _begin {
struct _fn {
    // TODO [必做] 1a：目前仅覆盖成员 begin()。
    // 真实 CPO 还需处理：数组退化（T (&arr)[N] → arr + 0）；
    // ADL begin(r)（放在 if constexpr 分支，成员优先于 ADL）。
    template<typename R>
        requires requires(R& r) { r.begin(); }
    auto operator()(R& r) const { return r.begin(); }

    // TODO [必做] 1a（续）：数组特化
    // template<typename T, std::size_t N>
    // T* operator()(T (&arr)[N]) const noexcept { return arr; }
};
} // namespace _begin
inline constexpr _begin::_fn begin{};

// ---------------------------------------------------------------------------
// end CPO
// ---------------------------------------------------------------------------
namespace _end {
struct _fn {
    // TODO [必做] 1b：对称实现 end CPO（成员 end() 优先；数组退化；ADL end(r)）
    template<typename R>
        requires requires(R& r) { r.end(); }
    auto operator()(R& r) const { return r.end(); }

    // TODO [必做] 1b（续）：数组特化
    // template<typename T, std::size_t N>
    // T* operator()(T (&arr)[N]) const noexcept { return arr + N; }
};
} // namespace _end
inline constexpr _end::_fn end{};

// ---------------------------------------------------------------------------
// 关联类型别名（依赖 begin/end CPO）
// ---------------------------------------------------------------------------
template<typename R>
using iterator_t = decltype(my::ranges::begin(std::declval<R&>()));

template<typename R>
using sentinel_t = decltype(my::ranges::end(std::declval<R&>()));

template<typename R>
using range_value_t = std::iter_value_t<iterator_t<R>>;

template<typename R>
using range_reference_t = std::iter_reference_t<iterator_t<R>>;

// ---------------------------------------------------------------------------
// iter_move CPO
// ---------------------------------------------------------------------------
namespace _iter_move {
struct _fn {
    // TODO [必做] 1c：
    // hidden-friend 定制：若 iter_move(it) 通过 ADL 可找到，优先调用。
    // 否则回退到 std::move(*it)（对非 proxy 迭代器等价于 *it 的右值）。
    template<typename I>
    decltype(auto) operator()(I&& it) const {
        if constexpr (requires { iter_move(std::forward<I>(it)); }) {
            return iter_move(std::forward<I>(it));
        } else {
            return std::move(*std::forward<I>(it));
        }
    }
};
} // namespace _iter_move
inline constexpr _iter_move::_fn iter_move{};

// ---------------------------------------------------------------------------
// size CPO（骨架，进阶 1d）
// ---------------------------------------------------------------------------
namespace _size {
struct _fn {
    // TODO [进阶] 1d：成员 size() 优先；否则 end - begin（要求 sized_sentinel_for）
    template<typename R>
        requires requires(R& r) { r.size(); }
    auto operator()(R& r) const { return r.size(); }
};
} // namespace _size
inline constexpr _size::_fn size{};

// ---------------------------------------------------------------------------
// enable_view / enable_borrowed_range 变量模板（concept 层使用）
// ---------------------------------------------------------------------------
template<typename T>
inline constexpr bool enable_view = false;

template<typename T>
inline constexpr bool enable_borrowed_range = false;

} // namespace my::ranges
