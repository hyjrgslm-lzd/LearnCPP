#pragma once
// =============================================================================
// my_ranges/02_concepts.hpp — 层 2：concept 子集骨架
//
// 对应章节：模块 F — concept 约束链
//
// 约束链（从宽到严）：
//   range → input_range → forward_range → bidirectional_range → random_access_range
//
// TODO 列表：
//   [必做] 2a. 补全 range concept（目前骨架已给出）
//   [必做] 2b. 补全 view concept（需要 enable_view 变量模板）
//   [必做] 2c. 补全 input_range / forward_range concept
//   [进阶] 2d. 增加 bidirectional_range / random_access_range / contiguous_range
//   [进阶] 2e. 增加 sized_range concept（需要 ranges::size CPO）
//   [进阶] 2f. 增加 borrowed_range concept
// =============================================================================

#include "01_cpo.hpp"
#include <iterator>
#include <concepts>

namespace my::ranges {

// ---------------------------------------------------------------------------
// range concept
// ---------------------------------------------------------------------------
// TODO [必做] 2a：range 要求能对 R& 调用 my::ranges::begin 和 my::ranges::end
template<typename R>
concept range = requires(R& r) {
    my::ranges::begin(r);
    my::ranges::end(r);
};

// ---------------------------------------------------------------------------
// view concept
// ---------------------------------------------------------------------------
// TODO [必做] 2b：view = range + movable + enable_view<remove_cvref_t<R>> = true
// 注意：std::movable = std::move_constructible + std::assignable + std::destructible
template<typename R>
concept view = range<R>
            && std::movable<R>
            && enable_view<std::remove_cvref_t<R>>;

// ---------------------------------------------------------------------------
// input_range concept
// ---------------------------------------------------------------------------
// TODO [必做] 2c：input_range = range + std::input_iterator<iterator_t<R>>
template<typename R>
concept input_range = range<R>
                   && std::input_iterator<iterator_t<R>>;

// ---------------------------------------------------------------------------
// forward_range concept
// ---------------------------------------------------------------------------
// TODO [必做] 2c（续）：forward_range = input_range + std::forward_iterator<iterator_t<R>>
template<typename R>
concept forward_range = input_range<R>
                     && std::forward_iterator<iterator_t<R>>;

// ---------------------------------------------------------------------------
// bidirectional_range concept（骨架，进阶 2d）
// ---------------------------------------------------------------------------
// TODO [进阶] 2d：
// template<typename R>
// concept bidirectional_range = forward_range<R>
//                            && std::bidirectional_iterator<iterator_t<R>>;

// ---------------------------------------------------------------------------
// random_access_range concept（骨架，进阶 2d）
// ---------------------------------------------------------------------------
// TODO [进阶] 2d：
// template<typename R>
// concept random_access_range = bidirectional_range<R>
//                            && std::random_access_iterator<iterator_t<R>>;

// ---------------------------------------------------------------------------
// contiguous_range concept（骨架，进阶 2d）
// ---------------------------------------------------------------------------
// TODO [进阶] 2d：
// template<typename R>
// concept contiguous_range = random_access_range<R>
//                         && std::contiguous_iterator<iterator_t<R>>;

// ---------------------------------------------------------------------------
// sized_range concept（骨架，进阶 2e）
// ---------------------------------------------------------------------------
// TODO [进阶] 2e：
// template<typename R>
// concept sized_range = range<R>
//                    && requires(R& r) { my::ranges::size(r); };

// ---------------------------------------------------------------------------
// borrowed_range concept（骨架，进阶 2f）
// ---------------------------------------------------------------------------
// TODO [进阶] 2f：
// template<typename R>
// concept borrowed_range = range<R>
//                       && (std::is_lvalue_reference_v<R>
//                           || enable_borrowed_range<std::remove_cvref_t<R>>);

// ---------------------------------------------------------------------------
// 验证点（main.cpp 中启用）
// ---------------------------------------------------------------------------
// static_assert(my::ranges::range<std::vector<int>>);
// static_assert(!my::ranges::view<std::vector<int>>);  // vector 不设 enable_view

} // namespace my::ranges
