# 设计文档：mini-ranges 子集实现

> 对应任务 5 / 验收点 6：不超过两页，解释每层的选择与简化点。

---

## 一、系统概述

mini-ranges 是 C++20 `std::ranges` 的教学子集，目标是端到端运行以下管道：

```cpp
auto v = my::views::iota(1, 11)
       | my::views::transform([](int x){ return x * x; })
       | my::views::take(5)
       | my::ranges::to<std::vector<int>>();
// v == {1, 4, 9, 16, 25}
```

系统包含六层，每层有明确的职责边界，层间依赖单向（上层 include 下层）。

---

## 二、各层设计选择

### 层 1：CPO 不用函数模板（ADL 隔离）

**选择**：`begin` / `end` / `iter_move` 实现为 `inline constexpr` 函数对象。

**理由**：
<!-- 填写：函数模板参与 ADL，第三方库若有同名 begin(T&) 自由函数，会在 ADL 查找时与标准实现竞争，
     导致意外劫持。函数对象不参与 ADL，调用 my::ranges::begin(r) 时精确找到 _fn::operator()，
     内部再决定走成员 begin() 还是 ADL，隔离语义更清晰（模块 E 核心论据）。 -->

---

### 层 2：enable_view 变量模板，不用 concept 直接检测成员

**选择**：`view` concept 依赖 `enable_view<remove_cvref_t<R>> = true` 的显式 opt-in。

**理由**：
<!-- 填写：vector<int> 有 begin/end 成员，但不应被视为 view（O(1) copy/move 公理不满足）。
     纯结构检测（有 begin/end = 是 range）无法区分 range 和 view；
     显式特化 enable_view = true 是程序员承诺"此类型满足 view 公理"的合约。 -->

---

### 层 3：view_interface 用 CRTP，不用虚函数

**选择**：`view_interface<D>` 通过 `static_cast<D&>(*this)` 调用派生类接口。

**理由**：
<!-- 填写：虚函数引入 vtable，破坏 O(1) copy/move（vtable 指针需同步），且阻止某些内联优化。
     CRTP 在编译期静态分发，零运行时开销，与 O(1) 公理完全兼容（模块 F 核心）。 -->

**range_adaptor_closure 同理**：
<!-- 填写：operator| 注入通过 CRTP friend 函数，每个 closure 类型只需继承基类，
     不重复实现 operator|，满足 P2387R3 的设计意图。 -->

---

### 层 5：take_view 的 sentinel 异型

**选择**：`take_view::end()` 返回 `sentinel` 类型，与 `begin()` 返回的 `iterator_t<V>` 不同。

**理由**：
<!-- 填写：C++20 iterator/sentinel 分离设计——end 不必是同类型的迭代器，
     只需满足 sentinel_for<S, I>（能与 iterator 比较）。
     sentinel 异型允许更精确地表达"到第 n 个元素停止"语义，
     同时让底层迭代器类型不变（无需包装成 counted_iterator 等重类型）。 -->

---

## 三、简化点与潜在问题

| 简化 | 影响 | 触发场景 |
|------|------|----------|
| `to<C>` 省略 `reserve` | 多次内存分配，性能次优 | 大范围收集时明显 |
| `closure \| closure` 用 lambda 而非具名 `_Pipe` | 影响 copyability 和编译期诊断质量 | 两个 closure 组合后再存储为变量时 |
| `take_view` 仅对 `random_access` 底层有效 | non-random-access 底层编译失败 | `filter_view \| take` 管道 |
| 不实现 `filter_view` 的 `__non_propagating_cache` | 缺少对"拷贝时重置"语义的验证 | filter 后的 copy 行为 |
| 不实现 const `begin/end` | `const view` 无法迭代 | `const` 传递的 range for 循环 |

---

## 四、与 stdlib 的主要差距

<!-- 填写：
  1. 不覆盖 C++20 全部 view（无 filter / join / zip / enumerate 等）
  2. 不实现 common_range 适配（无 common_view）
  3. 不实现 basic_const_iterator
  4. to<C> 不支持 insert-based 容器
  5. 不验证 iter_swap 的正确性
-->

---

## 五、完成后的理解

<!-- 完成所有 TODO 后，用 2-3 句话总结：
     从这个实现中，你对 P2387R3（range_adaptor_closure）有了什么新理解？
     哪一层最出乎意料地复杂？ -->

---

*填写完成后删除此行提示，保留所有章节内容。文档不应超过两页打印长度。*
