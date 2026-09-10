# 笔记：mini-ranges 六层架构图

层间依赖方向是单向的：上层 include 下层。学生只需要编辑 `src/student/my_ranges/` 下的六个 header；根目录 `my_ranges/` 只是旧入口导航，不保存第二套答案。

```text
┌───────────────────────────────────────────────────────────────┐
│ 层 6 — consumers：06_consumers.hpp                            │
│ my::ranges::to<C>()                                           │
│ 把惰性 range 收集到容器；管道最终出口。                         │
├───────────────────────────────────────────────────────────────┤
│ 层 5 — adaptors：05_adaptors.hpp                              │
│ transform_view / take_view / transform(...) / take(...)        │
│ view 保存底层 range；closure 保存参数并接入 operator|。          │
├───────────────────────────────────────────────────────────────┤
│ 层 4 — factories：04_factories.hpp                            │
│ iota_view / single_view                                       │
│ 无底层 view 的起点 range，负责产生管道源头。                     │
├───────────────────────────────────────────────────────────────┤
│ 层 3 — interface：03_interface.hpp                            │
│ view_interface<D> / range_adaptor_closure<D>                   │
│ 用 CRTP 注入 empty/front 与 range | closure。                   │
├───────────────────────────────────────────────────────────────┤
│ 层 2 — concepts：02_concepts.hpp                              │
│ range / view / input_range / forward_range / borrowed_range    │
│ 把结构能力和 view 公理分开表达。                                │
├───────────────────────────────────────────────────────────────┤
│ 层 1 — CPO：01_cpo.hpp                                        │
│ begin / end / size / iter_move + enable_view 变量模板          │
│ 统一访问入口，隔离 ADL，给上层 concept 与 view 使用。             │
└───────────────────────────────────────────────────────────────┘
```

## 层 1 — CPO

`begin` / `end` / `size` / `iter_move` 是 `inline constexpr` 函数对象。调用方写 `my::ranges::begin(r)`，先进入固定 CPO，再由 CPO 选择数组、成员函数或 ADL fallback。这样不会让第三方同名自由函数直接劫持顶层名字。

`enable_view<T>` 与 `enable_borrowed_range<T>` 默认是 `false`。具体 view 在对应 header 末尾显式特化，表示作者承诺该类型满足 view 或 borrowed_range 公理。

## 层 2 — concept

`range` 只要求能 `begin/end`；`view` 要求 `range + movable + enable_view<T>`。这能区分 `std::vector<int>` 和真正 view：vector 有 begin/end，但拷贝不是 O(1)，所以不能靠结构检测自动视为 view。

`input_range` / `forward_range` 继续把 iterator concept 叠上去，供上层 adaptor 限制能力。

## 层 3 — 基础设施

`view_interface<D>` 用 CRTP 调派到派生类，提供 `empty()` 和 `front()`。没有虚函数，没有 vtable，类型仍然保持标准 ranges 依赖的轻量值语义。

`range_adaptor_closure<D>` 注入 `range | closure`。每个 closure 只实现 `operator()(R&&)`，管道语法由基类统一提供。

## 层 4 — 工厂 view

`iota_view<int>` 的 iterator 只保存当前值，支持随机访问运算，并标记为 borrowed range。

`single_view<T>` 自己保存一个值，`begin/end` 返回裸指针。裸指针天然满足 contiguous/random access，但本 mini-ranges 只把它作为单元素 view 使用。

## 层 5 — adaptor view

`transform_view<V, F>` 的 iterator 保存底层 iterator 和函数指针，解引用时惰性调用函数。底层即使 contiguous，transform 后也只能按 random access 处理，因为解引用结果不再是底层连续内存里的元素引用。

`take_view<V>` 的 iterator 包装底层 iterator 并保存剩余数量，sentinel 保存底层 end。比较时“剩余数量为 0”或“到底层 end”任一条件成立即停止，因此既支持 `take(5)`，也支持底层提前结束。

## 层 6 — consumer

`to<C>()` 是最终物化出口。它要求容器支持 `push_back`，逐个消费输入 range。本题不做 `reserve`、insert-only 容器、嵌套 `to`、异常安全优化。

验收管道：

```cpp
auto v = my::views::iota(1, 11)
       | my::views::transform([](int x) { return x * x; })
       | my::views::take(5)
       | my::ranges::to<std::vector<int>>();
// v == {1, 4, 9, 16, 25}
```
