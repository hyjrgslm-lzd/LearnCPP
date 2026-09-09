#pragma once
// =============================================================================
// my_ranges/03_interface.hpp — 层 3：基础设施（view_interface + range_adaptor_closure）骨架
//
// 对应章节：模块 F（view_interface CRTP）/ 模块 G（range_adaptor_closure）
//
// 设计原则：
//   view_interface<D>：CRTP 基类，通过 static_cast<D&>(*this) 调用派生类
//     begin/end，注入 empty / front / back / size / operator[]。
//     零开销：无虚函数，无额外成员，完全编译期分发。
//
//   range_adaptor_closure<D>：CRTP 基类，注入 operator|（range | closure 路径）。
//     closure | closure 路径（_Pipe 组合）在进阶中实现，骨架已预留。
//
// TODO 列表：
//   [必做] 3a. 补全 view_interface::back()（需要 bidirectional_iterator）
//   [必做] 3b. 补全 view_interface::size()（需要 sized_sentinel_for）
//   [必做] 3c. 补全 view_interface::operator[]（需要 random_access_iterator）
//   [必做] 3d. 确认 range_adaptor_closure::operator| 的 range|closure 路径正确工作
//   [进阶] 3e. 实现 closure|closure 路径（_Pipe<C1,C2> 类型，本身也继承 range_adaptor_closure）
// =============================================================================

#include "02_concepts.hpp"
#include <utility>

namespace my::ranges {

// ---------------------------------------------------------------------------
// view_interface CRTP 基类
// ---------------------------------------------------------------------------
template<typename D>
struct view_interface {
private:
    // CRTP 辅助：安全获取派生类引用
    D& derived() noexcept { return static_cast<D&>(*this); }
    const D& derived() const noexcept { return static_cast<const D&>(*this); }

public:
    // empty()：begin == end 时为空
    // TODO [必做] 3a（前提）：此处已实现，确认与你的 begin/end CPO 集成正确
    bool empty() {
        return my::ranges::begin(derived()) == my::ranges::end(derived());
    }

    // front()：返回第一个元素
    // TODO [必做] 3a（前提）：已实现，确认 input_range 约束
    auto front() {
        return *my::ranges::begin(derived());
    }

    // back()：返回最后一个元素（需要 bidirectional_iterator）
    // TODO [必做] 3a：取消注释并补全 bidirectional_range concept 后启用
    // auto back()
    //     requires /* TODO: bidirectional_range<D> && common_range<D> */ false
    // {
    //     auto e = my::ranges::end(derived());
    //     return *--e;
    // }

    // size()：元素数量（需要 sized_sentinel_for）
    // TODO [必做] 3b：取消注释并补全 sized_range concept 后启用
    // auto size()
    //     requires /* TODO: forward_range<D> && sized_sentinel_for<sentinel_t<D>, iterator_t<D>> */ false
    // {
    //     return my::ranges::end(derived()) - my::ranges::begin(derived());
    // }

    // operator[]：随机访问（需要 random_access_iterator）
    // TODO [必做] 3c：取消注释并补全 random_access_range concept 后启用
    // auto operator[](std::ptrdiff_t n)
    //     requires /* TODO: random_access_range<D> */ false
    // {
    //     return my::ranges::begin(derived())[n];
    // }
};

// ---------------------------------------------------------------------------
// range_adaptor_closure CRTP 基类
// ---------------------------------------------------------------------------
template<typename D>
struct range_adaptor_closure {
    // operator|（range | closure 路径）
    // TODO [必做] 3d：验证此处的 friend operator| 与 iota | transform | take 管道正确组合
    template<my::ranges::range R>
    friend auto operator|(R&& r, const D& d) {
        return d(std::forward<R>(r));
    }

    // TODO [进阶] 3e：closure | closure 路径
    // 生成 _Pipe<D, C2>，本身也继承 range_adaptor_closure<_Pipe<D,C2>>
    // template<typename C2>
    //     requires std::derived_from<C2, range_adaptor_closure<C2>>
    // friend auto operator|(const D& c1, const C2& c2) {
    //     return _Pipe<D, C2>{c1, c2};
    // }
};

// ---------------------------------------------------------------------------
// _Pipe：closure 组合类型骨架（进阶 3e）
// ---------------------------------------------------------------------------
// TODO [进阶] 3e：实现 _Pipe<C1,C2>，继承 range_adaptor_closure<_Pipe<C1,C2>>
// template<typename C1, typename C2>
// struct _Pipe : range_adaptor_closure<_Pipe<C1, C2>> {
//     C1 _c1; C2 _c2;
//     _Pipe(C1 c1, C2 c2) : _c1(std::move(c1)), _c2(std::move(c2)) {}
//     template<my::ranges::range R>
//     auto operator()(R&& r) const { return std::forward<R>(r) | _c1 | _c2; }
// };

} // namespace my::ranges
