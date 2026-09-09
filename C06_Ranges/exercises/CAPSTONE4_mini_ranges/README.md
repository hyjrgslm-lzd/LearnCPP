# 结课项目 4：mini-ranges 子集实现

> 对应文件：`12-结课项目2-实现级源码阅读.md` §"结课项目 4：mini-ranges 子集实现"
> 阶段定位：阶段二（实现层）结课项目，综合考察模块 E-H 全部实现技术。

---

## 项目目标

从零实现最小但可用的 mini-ranges 子系统。综合考察阶段二全部实现技术。
mini-ranges 是子集，不是完备实现，不覆盖 C++23 全部 view，
但对象关系和编译期语义必须正确。

---

## 架构总览

```
┌──────────────────────────────────────────────────────────┐
│ 层 6  消费层     06_consumers.hpp                        │
│       ranges::to<C>                                      │
│       继承 range_adaptor_closure，可出现在管道右侧        │
├──────────────────────────────────────────────────────────┤
│ 层 5  closure 层  05_adaptors.hpp                        │
│       _transform_closure / _take_closure                 │
│       持有参数，等待 range 输入，注入 operator|           │
├──────────────────────────────────────────────────────────┤
│ 层 4  view 层   04_factories.hpp + 05_adaptors.hpp       │
│       iota_view / single_view                            │
│       transform_view / take_view                         │
│       继承 view_interface，惰性求值                       │
├──────────────────────────────────────────────────────────┤
│ 层 3  基础设施层  03_interface.hpp                        │
│       view_interface<D>   — CRTP，注入 empty/front/...   │
│       range_adaptor_closure<D> — CRTP，注入 operator|   │
├──────────────────────────────────────────────────────────┤
│ 层 2  概念层    02_concepts.hpp                          │
│       range / view / input_range / forward_range        │
│       编译期约束，不产生运行时代码                         │
├──────────────────────────────────────────────────────────┤
│ 层 1  CPO 层    01_cpo.hpp                               │
│       begin / end / iter_move / size                    │
│       inline constexpr 函数对象，ADL 隔离               │
│       enable_view / enable_borrowed_range 变量模板       │
└──────────────────────────────────────────────────────────┘
       依赖方向：上层 include 下层（单向）
```

---

## 必做任务

### 任务 1（`main.cpp` TODO [必做] 1 + `notes/architecture_diagram.md`）：先画架构图

实现之前，先补全 `notes/architecture_diagram.md` 的六层依赖图，
用文字说明每层职责和层间依赖方向。

**交付**：`notes/architecture_diagram.md` 六层说明全部填写，`main.cpp` TODO 1 验证通过。

### 任务 2（`main.cpp` TODO [必做] 2 + TODO [必做] 3）：按底层到上层顺序实现

按架构图六层从下到上：CPO → concept → 基础设施 → iota/single → transform/take → to<C>。
每完成一层，取消注释对应 static_assert 验证通过，再进入下一层。

**交付**：`main.cpp` TODO 1-4 验证块的 static_assert 全部通过。

### 任务 3（`main.cpp` TODO [必做] 3）：每层至少一条 static_assert

必须覆盖：
- `my::ranges::range<vector<int>>`
- `my::ranges::view<iota_view<int>>`
- `!my::ranges::view<vector<int>>`
- `enable_borrowed_range<iota_view<int>>`
- `!same_as<decltype(tv.begin()), decltype(tv.end())>`（sentinel 异型）

**交付**：以上五条 static_assert 全部通过。

### 任务 4（`main.cpp` TODO [必做] 4）：最终样例编译运行

```cpp
auto v = my::views::iota(1, 11)
       | my::views::transform([](int x){ return x * x; })
       | my::views::take(5)
       | my::ranges::to<std::vector<int>>();
assert((v == std::vector<int>{1, 4, 9, 16, 25}));
```

**交付**：`main.cpp` TODO 5 验证块通过，assert 不触发。

### 任务 5（`notes/design_doc.md`）：写设计文档

解释每层的选择：CPO 不用函数模板（ADL 隔离）；view_interface 用 CRTP 不用虚函数；
range_adaptor_closure 用 CRTP 不手写 operator|；take_view sentinel 异型；
mini-ranges 的简化点与潜在问题。

**交付**：`notes/design_doc.md` 所有章节填写完整，不超过两页打印长度。

---

## 进阶任务

### 进阶 1：std::ranges::copy 消费 mini-ranges view（stdlib 互操作）

```cpp
auto my_view = my::views::iota(1, 6)
             | my::views::transform([](int x){ return x * 2; });
std::vector<int> out;
std::ranges::copy(my_view, std::back_inserter(out));
// out == {2, 4, 6, 8, 10}
```

验证"成员优先 CPO 设计"的直接收益：只要迭代器满足 `std::input_iterator`，stdlib 算法就能消费。
（对应 `main.cpp` TODO [进阶] 1）

### 进阶 2：增加 `my::views::filter` + `__non_propagating_cache`

实现等价的 non-propagating optional wrapper；begin() 非 const；
验证"拷贝不传播、移动传播"；验证 `const filter_view` 不能调用 begin()。
（对应 `my_ranges/05_adaptors.hpp` TODO [进阶] 5f，`main.cpp` TODO [进阶] 2）

### 进阶 3：增加 `my::views::enumerate` 并让其在底层 borrowed 时也 borrowed

`enumerate_view<V>::enable_borrowed_range` 特化：当且仅当底层 V 是 borrowed_range 时为 true。
（对应 `main.cpp` TODO [进阶] 3）

### 进阶 4：实现 iterator_concept 双轨

在 `transform_view::iterator` 中完整实现 `iterator_concept` vs `iterator_category` 两套标签，
用 static_assert 验证两者在 proxy reference 场景下的不同值。
（对应 `main.cpp` TODO [进阶] 4）

---

## 最终验收样例代码

```cpp
// 填写实现后，在 main.cpp 中取消注释此块并运行：
auto v = my::views::iota(1, 11)
       | my::views::transform([](int x){ return x * x; })
       | my::views::take(5)
       | my::ranges::to<std::vector<int>>();
assert((v == std::vector<int>{1, 4, 9, 16, 25}));
std::puts("CAPSTONE4: pipeline OK");
```

---

## 验收点

1. `my::ranges::begin` 是 `inline constexpr` 函数对象，能作为值存储，不是函数模板
2. `my::ranges::view` concept 对 `iota_view<int>` 成立，对 `std::vector<int>` 不成立
3. 管道 `iota(1,11) | transform(x*x) | take(5) | to<vector<int>>()` 编译运行，结果为 `{1, 4, 9, 16, 25}`
4. `take_view` 的 `end()` 返回类型与 `begin()` 返回类型不同（sentinel 异型）
5. `iota_view` 的 `enable_borrowed_range = true`；`transform_view` 的为 `false`（默认）
6. 能拿着六层架构图解释整个系统，指出每层的职责和层间依赖方向

---

## 复盘问题

1. 实现过程中，哪一层最出乎意料地复杂？CPO ADL 隔离、range_adaptor_closure 的
   closure-to-closure 组合，还是 to<Container> 的通用性约束？
2. `take_view` 的 sentinel 异型设计，在 `to<Container>` 消费端带来了什么额外要求？
3. 如果要增加 `my::views::filter`，begin cache 的 non-propagating 语义如何影响
   copy 行为的测试设计？
4. 你的 mini-ranges 与 stdlib 相比，最大的简化在哪里？这些简化会在什么场景下
   导致错误（而不只是缺失功能）？
5. 完成这个项目后，对 P2387R3 的设计选择有什么新理解？为什么标准选择 CRTP 基类
   而非要求用户手写 `operator|`？

---

## 对应官方参考

| 资源 | 内容 |
|------|------|
| P2387R3 | `range_adaptor_closure` 正式化设计 |
| P0896R4 | ranges 总体设计，borrowed_range 引入 |
| cppreference `std::ranges::transform_view` | inner iterator 结构参考 |
| cppreference `std::ranges::take_view` | sentinel 异型实现参考 |
| libstdc++ `<bits/ranges_adaptors.h>` | transform/filter/join 实现对照 |
| cppreference `std::ranges::to` (C++23) | to<C> 完备实现参考 |
