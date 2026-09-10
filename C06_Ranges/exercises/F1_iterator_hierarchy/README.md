> 对应章节：../../09-模块F-概念精化与迭代器分类.md 练习 F-1：自定义 forward → bidirectional → random_access → contiguous iterator

## 目标

从零构造一个最小 iterator，逐步满足六层 concept 要求，把"concept 链是叠加精化（refinement）而不是替代（OR）"刻进直觉。同时实现 P1207R4 风格的 move-only iterator，验证 `input_iterator` 不再要求 copyable。

## 前置理解

- **concept 链是 refinement**：`forward_iterator` 是 `input_iterator` 的细化，不是替代。满足 `random_access_iterator` 的类型同时满足 `bidirectional_iterator`、`forward_iterator`、`input_iterator`——叠加精化，不是互换。
- **`iterator_concept` 与 `iterator_category` 双轨**：`iterator_concept` 供 C++20 concept 系统读取；`iterator_category` 供 C++17 算法的 `iterator_traits` 读取。两者不存在时，concept 系统会从 `iterator_category` 反推。当需要"C++20 能力 > C++17 能力"时（如 proxy reference），才必须显式分开声明。
- **`forward_iterator` 要求 copyable**：因为 multipass guarantee——forward iterator 需要能保存当前位置并重复遍历（多次传递同一序列）。`input_iterator` 不要求（P1207R4）。
- **`static_assert` 是主要验证手段**：concept 是编译期断言，比运行期行为更重要。

## 必做任务

1. **阶段 1：`forward_iterator`**。定义 `my_range_fwd::iterator`，提供五个类型别名（`value_type`、`reference`、`difference_type`、`iterator_category`、`iterator_concept`，均为 `forward_iterator_tag`）、`operator*()`、前置/后置 `operator++`、`operator==`、默认构造。`static_assert` 验证 `forward_iterator` 蕴含 `input_iterator`，不蕴含 `bidirectional_iterator`。

2. **阶段 2：`bidirectional_iterator`**。在 forward 基础上增加前置/后置 `operator--`，tag 改为 `bidirectional_iterator_tag`。注意：后置 `operator--` 返回类型必须是 `iterator`（右值），不能是 `void`。`static_assert` 验证三层 concept 同时成立。

3. **阶段 3：`random_access_iterator`**。增加 `operator+=`、`operator-=`、`operator+`（两个方向）、`operator-`（iter - n）、`operator-`（iter - iter，返回 `difference_type`）、`operator[]`（下标访问）、`operator<=>`（spaceship，一次提供全套有序比较）。`static_assert` 验证四层 concept 同时成立。

4. **阶段 4：`contiguous_iterator`**。将 `iterator_concept` 改为 `contiguous_iterator_tag`（`iterator_category` 保持 `random_access_iterator_tag`），增加 `operator->()` 返回裸指针。`static_assert` 验证 `contiguous_iterator` 和 `contiguous_range`。

5. **验证 concept 是 refinement**。实现 `demo_forward_algo`：接受 `forward_iterator`，传入 `random_access_iterator` 完全合法——因为 random_access 蕴含 forward。

6. **每步对比需要新增的接口**，理解增量要求表（见 09 章节"必做任务 6"的表格）。

## 进阶任务

- **P1207R4 move-only iterator**：实现 `move_only_input_it`，删除拷贝构造（`= delete`），只保留移动构造。`static_assert(std::input_iterator<move_only_input_it>)` 通过；`static_assert(!std::forward_iterator<move_only_input_it>)` 通过。这演示了 P1207R4 打破"迭代器必须 copyable"的历史意义。
- **双轨不一致实验**：把 `iterator_concept` 设为 `random_access_iterator_tag`，`iterator_category` 设为 `forward_iterator_tag`。观察 `std::ranges::sort`（使用 `iterator_concept`）编译通过，而 `std::sort`（使用 `iterator_category`）退化行为。
- **为 `demo_forward_algo` 加精确 concept 约束**：对比有无约束时编译错误信息的差异（有约束时错误更精确）。

## 验收点

- 能从零写出满足四层 concept 的自定义迭代器（forward / bidirectional / random_access / contiguous）。
- 每层都有 `static_assert` 证明同时满足所有低层 concept（refinement 方向）。
- 能说清楚为什么 `forward_iterator` 要求 copyable 而 `input_iterator` 不要求。
- 能对比每层需要新增的运算符/类型别名（增量表）。
- move-only iterator 满足 `input_iterator` 但不满足 `forward_iterator`，能用 `static_assert` 证明。

## 观察点

- concept 链是叠加的：`random_access_iterator` 同时满足下面所有层。这不是巧合，这是 refinement 的定义——高层 concept 包含低层 concept 的所有约束。
- `iterator_concept` 是可选 typedef：若不存在，concept 推导从 `iterator_category` 反推（`random_access_iterator_tag` → `random_access_iterator` concept）。只有"C++20 能力 > C++17 能力"时才必须显式声明。
- 忘记 `difference_type` 或 `value_type` 会让 concept 直接失败，错误信息很长——学会从末尾的 "note:" 找到真正缺失的约束。
- `operator<=>` 是 `random_access_iterator` 的必要条件之一，只有 `==` 是不够的。

## 常见坑

- **忘记五个类型别名**：缺少 `difference_type` / `value_type` 会让 concept 直接失败，错误信息极难读懂。
- **后置 `operator--` 返回 `void`**：`bidirectional_iterator` 的 concept 约束检查后置 `operator--` 的返回类型必须是 `iterator`（右值），返回 `void` 不满足。
- **只加 `+=` / `-=` 忘记 `operator[]`**：`random_access_iterator` 还要求 `operator[]`，遗漏会导致 concept 失败。
- **以为只加 `iterator_concept = contiguous_iterator_tag` 就满足 `contiguous_iterator`**：还需要 `operator->()` 返回裸指针。
- **`operator==` const 版本 type mismatch**：如果定义了 const 迭代器，需要为两种组合提供 `operator==`。

## 复盘问题

- 为什么 `forward_iterator` 要求默认可构造，而 `input_iterator` 不要求？（提示：multipass guarantee——forward 需要能保存位置并重放。）
- `random_access_iterator` 增加的 `operator[]` 和 `operator<=>` 各自解决了什么问题？
- 如果把 `iterator_category` 设为 `random_access_iterator_tag` 但不提供 `operator-(iter, iter)`，会发生什么？
- `contiguous_iterator` 和 `random_access_iterator` 的语义区别是什么？对 SIMD 优化有什么影响？

## 对应官方参考

- P0896R4：C++20 ranges 六层迭代器 concept 链定义
- P1207R4：move-only iterator 合法化（`input_iterator` 不再要求 copyable）
- P1614R2：spaceship 运算符集成（`random_access_iterator` 的 `<=>` 要求）
- cppreference: [iterator concepts](https://en.cppreference.com/w/cpp/iterator#Iterator_concepts)

## Author-validation layout

本题已迁移到统一四路径验证：`checks/main.cpp` 只依赖 `#include <iterator_hierarchy.hpp>` 暴露的 `c06_f1::forward_range`、`bidirectional_range`、`random_access_range`、`contiguous_range` 和 `move_only_input_iterator`。Reference 与 good 各自独立实现；bad 是可编译、可安全运行的错误实现，故意让 random access distance 多 1；Student 可编译但 begin 偏移，不能被 checker 标记通过。

checker 不只看 typedef 名字，而是实际消费接口：遍历 forward、反向走 bidirectional、使用 random-access `[]` / `+` / `-`、用 `std::to_address` 验证 contiguous，并验证 move-only iterator 满足 input 而非 forward。声明了某个 iterator concept，就必须提供该 concept 对应的真实操作。
