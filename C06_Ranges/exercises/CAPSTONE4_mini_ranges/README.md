# 结课项目 4：mini-ranges 子集实现

> 对应章节：`12-结课项目2-实现级源码阅读.md` / CAPSTONE4。

## 项目目标

从零实现一个受限但可运行的 mini-ranges 子系统，贯穿模块 E-H 的实现主题：CPO、concept、CRTP、view、adaptor closure、iterator/sentinel、consumer。

本项目不允许学生用 `std::views` 包一层冒充自写实现。标准库可以在 checker 或 `validation/good` oracle 里作为行为参照；Reference 和 Student 路径必须保留自写六层实现。

## 文件路径

- `src/student/my_ranges/01_cpo.hpp` 到 `06_consumers.hpp`：学生唯一编辑入口。
- `src/reference/my_ranges/`：标准答案。
- `validation/good/my_ranges/`：独立 oracle。它用标准库范围设施加一层兼容外壳验证公共行为，不复用 Reference 的六层源码。
- `validation/bad/my_ranges/`：安全但真实错误的实现，`take_view` 多取一个元素，必须被 checker 拒绝。
- 根目录 `my_ranges/`：旧入口兼容导航，只 include 到 `src/student/my_ranges/`，不要在这里写第二套答案。

## 六层结构

```text
层 6  consumers       06_consumers.hpp    my::ranges::to<C>()
层 5  adaptors        05_adaptors.hpp     transform_view / take_view / closure
层 4  factories       04_factories.hpp    iota_view / single_view
层 3  interface       03_interface.hpp    view_interface / range_adaptor_closure
层 2  concepts        02_concepts.hpp     range / view / input_range / forward_range
层 1  CPO             01_cpo.hpp          begin / end / size / iter_move
```

依赖方向固定为上层 include 下层。先完成底层，再让上层 concept、view、adaptor 和 consumer 使用它。

## 必做任务

1. CPO 层。
   - `my::ranges::begin/end` 支持数组、成员函数和 ADL fallback。
   - `my::ranges::iter_move` 支持 hidden-friend 定制，失败时回退到 `std::move(*it)`。
   - `enable_view` / `enable_borrowed_range` 默认 `false`。

2. concept 层。
   - `range` 只检测 begin/end。
   - `view` 必须显式 opt-in，不能把 `std::vector<int>` 当 view。
   - `input_range`、`forward_range` 基于 iterator concept 逐级加约束。

3. CRTP 基础设施。
   - `view_interface<D>` 提供 `empty()` / `front()`。
   - `range_adaptor_closure<D>` 提供 `range | closure`。

4. 工厂 view。
   - `iota_view<int>` 支持随机访问 iterator，标记 borrowed。
   - `single_view<T>` 保存一个对象并返回指针 begin/end。

5. adaptor view。
   - `transform_view<V, F>` 惰性调用函数，保留合理 iterator 能力。
   - `take_view<V>` 使用 wrapper iterator + sentinel，停止于“取够数量”或“底层结束”。

6. consumer。
   - `my::ranges::to<C>()` 用 `push_back` 收集 range。
   - 支持最终管道：

```cpp
auto v = my::views::iota(1, 11)
       | my::views::transform([](int x) { return x * x; })
       | my::views::take(5)
       | my::ranges::to<std::vector<int>>();
// {1, 4, 9, 16, 25}
```

## 验收点

- CPO 是函数对象，可作为值使用。
- `std::vector<int>` 是 range，但不是 view。
- `iota_view<int>` 是 view 和 borrowed range。
- `transform_view` 能被 `std::ranges::copy` 消费。
- `take_view` 的 begin/end 是异型 iterator/sentinel。
- `take(99)` 遇到底层提前结束时不会越界。
- `validation/bad` 的 off-by-one `take_view` 被 checker 拒绝。

## 受限边界

本题不实现完整标准 ranges。下列简化是刻意保留的学习边界：

- 不实现 `filter_view`、`join_view`、`zip_view`、`common_view`、`as_const`。
- 不实现 `closure | closure` 的具名组合对象。
- `to<C>()` 只支持 `push_back` 容器，不处理 map/set、allocator、reserve 优化。
- 不追求完整 `noexcept`、borrowed propagation 和所有 category 细节。

进阶实现 `filter_view + __non_propagating_cache` 时，注意 C++26 新增的是 input-only const 分支；普通 forward 底层的缓存分支仍不能泛化成 const begin。

## 笔记交付

- `notes/architecture_diagram.md`：六层架构图和每层职责。
- `notes/design_doc.md`：关键设计选择、支持范围和简化边界。
