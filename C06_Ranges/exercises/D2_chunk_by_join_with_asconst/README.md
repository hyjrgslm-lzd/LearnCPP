# 练习 D-2：chunk_by / join_with / as_const / as_rvalue

> 对应章节：../../06-模块D-C++23高阶视图与协程桥.md §练习 D-2
> 提案：P2442R1 (chunk_by) / P2441R2 (join_with) / P2278R4 (as_const) / P2446R2 (as_rvalue)
> C++ 标准：C++23

## 目标

覆盖四个"改变语义而非几何形状"的 C++23 适配器：按相邻元素条件分块、带分隔符展平、只读包装、移动语义包装。

## 前置理解

- 已完成模块 C1 练习 C1-1，理解 `views::join` 的展平行为和分隔符不插入的语义。
- 已了解 `filter_view` 的 const 迭代问题：`filter_view` 的 `begin()` 是非 const 成员，`as_const` 不修复这个限制（两者正交）。
- 接受这题的重点是四个适配器各自的语义边界，以及它们常见的误用方式。

## 必做任务方向

1. **chunk_by 基础**：`v = {1,2,3,1,2,4,5}`，用 `chunk_by([](a,b){ return a <= b; })` 分块，手动画出期望边界再运行对照。
2. **chunk_by RLE**：`w = {1,1,2,2,2,3,1,1}`，用 `chunk_by([](a,b){ return a == b; })` 产出游程分组。
3. **chunk_by vs chunk(n)**：对同一序列对比两种分块的结果，体会"按内容 vs 按大小"的差异。
4. **join_with 单字符**：`words = {"hello","world","cpp23"}`，`join_with('-')` 产出 `hello-world-cpp23`，对比 `join` 的无分隔符输出。
5. **join_with 字符串分隔符**：`join_with(std::string{", "})` 产出 `hello, world, cpp23`。
6. **as_const 验证**：用 `static_assert` 验证 `range_reference_t == const int&`，尝试（注释中）写修改操作观察编译错误。
7. **as_const + zip**：演示用 `as_const` 保护 zip 的只读端，证明 const 保护生效。
8. **as_rvalue + ranges::to**：把 `vector<string>` 通过 `as_rvalue | ranges::to<vector<string>>()` move 进新容器，打印 move 前后状态。

## 进阶预计方向

1. 用 `chunk_by + transform` 实现 RLE 编码：`{a,a,b,b,b,c}` → `(a,2) (b,3) (c,1)`。
2. 演示 `as_const` 与 `filter_view` const 迭代问题正交：套了 `as_const` 后 `filter_view` 的 `begin()` 仍然是非 const 成员。
3. 对比 `for (auto s : strs | as_rvalue)` 与 `for (const auto& s : strs | as_rvalue)` 后 `strs` 元素的状态差异（前者触发 move，后者不触发）。

## 验收点

- `chunk_by` 的分块代码正确产出预期结果，能说出断点判定逻辑（`pred` 返回 `false` 的位置是块边界）。
- `join_with` 与 `join` 的输出差异可以通过运行结果直接观察到。
- 用 `static_assert` 证明 `as_const` 把元素引用类型变成了 `const T&`。
- 能说出 `as_rvalue` 为何不"自动 move 所有元素"：只有消费端发生赋值/构造时才会 move。
- 能说清楚 `as_const` 与 `filter_view` const 迭代问题的正交关系。

## 观察点

- `chunk_by` 的谓词 `pred(a, b)` 语义是"a 和 b 是否应该在同一块"——遇到第一个使 `pred(prev, curr)` 为 `false` 的 `curr` 时，在 `prev` 之后切断，`curr` 开始新块。这与字面理解相反，容易出错。
- `join_with` 是惰性的：分隔符不会被提前物化到内存。对 `vector<string>` 使用 `join_with('-')` 时，迭代器在相邻字符串之间"插入"一个虚拟的 `'-'` 字符，不产生中间字符串。
- `as_const` 内部使用 `basic_const_iterator`（P2278R4 引入），它把底层迭代器的 `operator*` 返回类型转换为 `const` 引用。
- `as_rvalue` 的 `operator*` 返回 `std::move(*inner_it)`（右值引用）。`auto s = *it` 触发移动构造；`auto& s = *it` 编译失败；`auto&& s = *it` 绑定转发引用，不一定 move。

## 常见坑

- **把 as_const 当 filter_view 的 const 迭代修复**：`as_const` 改变元素的 const 性，不改变 view 本身的 `begin()` 是否是 const 成员。`filter_view` 的 `begin()` 是非 const 成员，套了 `as_const` 也无法对 const `filter_view` 调用 `begin()`。
- **把 as_rvalue 当"自动 move 所有元素"**：`as_rvalue` 只产出右值引用，move 动作发生在消费侧。若消费只是读取（`const auto& s`），元素不会被 move 走。
- **chunk_by 的谓词方向混淆**：`chunk_by([](a,b){ return a < b; })` 意思是"只要 a < b 就在同一块"——第一个 a >= b 的位置切断，不是"在满足条件的位置切断"。
- **对 as_rvalue 的输出 range 做二次迭代**：如果上一次迭代已经通过 move 消费了元素（如 `ranges::to`），原始容器中的元素已处于移后状态，二次迭代读到的是空字符串（有效但未指定值）。

## 提示

- 验证 `chunk_by` 的分块时，先手动画出期望的块边界，再运行代码对照，可以快速发现谓词方向是否写反。
- 测试 `as_const` 的 const 保护时，故意写 `for (int& x : cv) { x = 0; }` 观察编译错误信息——错误里会出现 `const int&` 无法绑定到 `int&`。
- `as_rvalue` 与 `ranges::to` 组合是最常见的使用场景：先确认 `ranges::to` 使用移动构造（C++23 P1206R7），再用 `as_rvalue` 消除冗余拷贝。

## 复盘问题

1. `chunk_by(pred)` 在处理空序列时产出什么？产出单元素序列时产出什么？（边界情况）
2. `join_with` 产出的 view 是 `common_range` 吗？它的迭代器概念由什么决定？
3. `as_const` 返回的 view 是 `borrowed_range` 吗？底层 vector 销毁后，迭代器是否悬垂？
4. 如果你有一个 `vector<unique_ptr<T>>`，用 `as_rvalue` 配合 `ranges::to` 可以做什么？不能做什么？

## 对应官方参考

- P2442R1：C++23 `views::chunk_by`（相邻条件分块）
- P2441R2：C++23 `views::join_with`（带分隔符展平）
- P2278R4：`views::as_const` 与 `basic_const_iterator`
- P2446R2：C++23 `views::as_rvalue`
- cppreference：[`std::ranges::chunk_by_view`](https://en.cppreference.com/w/cpp/ranges/chunk_by_view)
- cppreference：[`std::ranges::join_with_view`](https://en.cppreference.com/w/cpp/ranges/join_with_view)
- cppreference：[`std::ranges::as_const_view`](https://en.cppreference.com/w/cpp/ranges/as_const_view)
- cppreference：[`std::ranges::as_rvalue_view`](https://en.cppreference.com/w/cpp/ranges/as_rvalue_view)
## 参考解析

预测：`chunk_by` 按相邻元素关系分组，不是按固定长度；`join_with` 在拍平时插入分隔符；`as_const` 改变元素引用的 const 性，不等于让所有 view 都具备 const begin；`as_rvalue` 把元素暴露为可移动引用。

当前程序验证相邻相等游程分组、相邻非递减分组、字符串 join_with 分隔、as_const 的 `const int&` 引用类型，以及 as_rvalue 移出字符串。扩展时应分别观察“元素 const”和“view 对象 const 可迭代性”，两者不要混淆。
