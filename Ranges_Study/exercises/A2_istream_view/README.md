> 对应章节：../../02-模块A-视图工厂与惰性.md §练习 A-2

## 目标

观察 `input_range` 的单遍性（single-pass）：`begin()` 不幂等，同一个流不能被
第二次消费；理解为什么 `istream_view` 的迭代器只能建模 `input_iterator`；
体会 P1207R4 引入的 move-only iterator 语义对"不能有两个独立推进副本"的
类型层面强制。

## 前置理解

- 理解 `forward_range` 要求迭代器可以被复制（multi-pass 保证）：在同一位置保存
  多个迭代器副本，推进其中一个不影响其他副本。
- 理解 `input_range` 只要求单遍：每个元素只能被读取一次，迭代器推进后之前的副本
  不再有效。
- 了解 P1207R4 的核心贡献：允许 C++20 `input_iterator` 是 move-only 的（不必
  copyable），打破了 C++17 `InputIterator` 必须 copyable 的限制。

## 预计练习方向

- 构造 `istream_view<int>`，用 `static_assert` 验证：
  - 满足 `input_range`，不满足 `forward_range`
  - 不满足 `sized_range`（流大小未知），不满足 `common_range`（sentinel end）
- 演示单遍性：第一次 for-range 消费所有元素；第二次对同一 view 迭代，
  什么都不产出（流已到 EOF）。
- 验证迭代器的 `iterator_concept` 是 `input_iterator_tag`，
  且迭代器不满足 `std::copyable`（move-only，P1207R4 的结果）。
- 用 `ranges::copy` + `back_inserter` 把 `istream_view` 写入 `vector`。

## 进阶预计方向

- 用 C++23 `ranges::to<vector<int>>()` 替代 `copy` + `back_inserter`，
  对比两种写法的语义（算法 vs 容器化操作）。
- 通过 `istringstream::tellg()` 在 for-range 前后打印流位置，
  直接证明 `begin()` 不幂等：for-range 结束后流位置推进到 EOF（-1）。
- 思考：如果把 `istream_view` 传给需要 `forward_range` 的算法（如 `ranges::count`），
  会是编译期错误还是运行期错误？

## 验收点

- 能用代码证明"第二次对同一 `istream_view` 迭代什么都不产出"，
  并解释背后的原因（流状态机制 + `begin()` 不幂等）。
- 能解释为什么 `istream_view` 的 iterator 是 move-only 的，
  以及 P1207R4 相对于 C++17 放宽了什么约束。
- 能区分 `iterator_concept = input_iterator_tag` 与 `iterator_category` 的含义，
  说出两者在 `istream_view` 的 iterator 上是否相同。
- 所有 `static_assert` 编译通过，无需注释任何一行。

## 观察点

- `istream_view` 是视图工厂中唯一真正"消耗外部资源"的 view：数据来自 `istream` 对象，
  而 `istream` 的读取是有状态的单向前进操作。
- `input_iterator` 与 `forward_iterator` 的本质区别不是"能否递增"，
  而是 **multi-pass 保证**：`forward_iterator` 保证可以保存副本并重新遍历相同序列，
  `input_iterator` 不提供这个保证。
- move-only iterator（P1207R4）使 `istream_view` 的 iterator 无法被复制，
  从类型层面强制了"不能有两个独立推进的副本"的语义约束。
- `ranges::copy` 与 `ranges::to` 的定位：前者是算法（把元素复制到已有目标），
  后者是容器化操作（从 range 构造容器）；对 `input_range` 都只能单遍消费。

## 常见坑

- 期待 `istream_view` 像 `iota_view` 一样可以被多次遍历：`iota_view` 每次
  for-range 都从同一起始值重新生成；`istream_view` 绑定到流对象，流状态不重置。
- 把 `begin()` 当作幂等操作调用多次：`istream_view::begin()` 触发第一次读取，
  多次调用不会回到序列起始位置。
- 期待 iterator 可以被复制然后独立推进：move-only iterator 在尝试复制时触发编译错误，
  这是刻意的类型级保护。
- 误解 P1207R4 的范围：P1207R4 只放宽了 `input_iterator` 的 copyable 要求；
  `forward_iterator` 及以上仍然要求 copyable，因为 multi-pass 保证依赖于迭代器副本的独立性。

## 提示

- 验证 iterator 是 move-only：
  ```cpp
  using It = std::ranges::iterator_t<decltype(iv1)>;
  static_assert(!std::copyable<It>);
  static_assert(!std::copy_constructible<It>);
  ```
- 验证 `begin()` 不幂等：在 for-range 前后打印 `iss.tellg()`，观察位置变化。
- 区分"流到达 EOF"和"view 为空"：第二次 for-range 之所以什么都不产出，
  是因为底层流已到 EOF，`operator>>` 失败，`istream_view` 的 iterator 立即等于 sentinel。
- `ranges::to<>` 是 C++23 特性，需要 `/std:c++latest` 或 C++23 编译选项。

## 复盘问题

1. 为什么"multi-pass 保证"是 `forward_iterator` 的核心语义，
   而不只是"可以递增两次"？
2. `istream_view` 的 `begin()` 和 CPO `ranges::begin()` 之间的关系是什么？
   CPO 最终会调用到 view 的哪个成员？
3. 如果把 `istream_view` 传给需要 `forward_range` 的算法，
   会发生编译期错误还是运行期错误？为什么？
4. P1207R4 允许 move-only iterator 建模 `input_iterator`，
   这个改变对哪些接受 `InputIterator` 的 C++17 算法有影响？

## 对应官方参考

- P0896R4：C++20 ranges 基础，包含 `istream_view` 的初始设计
- P1207R4：Movability of single-pass iterators（move-only `input_iterator` 的合法化）
- P1035R7：C++20 ranges 对标准算法的概念化改造
- cppreference：[`std::ranges::istream_view`](https://en.cppreference.com/w/cpp/ranges/istream_view)
- cppreference：[`std::ranges::input_range`](https://en.cppreference.com/w/cpp/ranges/input_range)
- cppreference：[`std::input_iterator`](https://en.cppreference.com/w/cpp/iterator/input_iterator)
