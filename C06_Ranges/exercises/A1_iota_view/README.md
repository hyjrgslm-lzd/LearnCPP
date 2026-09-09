> 对应章节：../../02-模块A-视图工厂与惰性.md §练习 A-1

## 目标

观察 `iota_view` 在有界与无界两种形态下，`sized_range` / `common_range` /
`random_access_range` / `iterator_concept` 的差异；用 `unreachable_sentinel_t`
体会 sentinel 与 iterator 分离的价值；用 `views::take` 把无界序列变为有界序列。

## 前置理解

- 理解 `01-心智模型.md` 中"iterator + sentinel"一节：`begin()` 和 `end()` 可以是
  完全不同的类型，`end()` 只需能与 iterator 用 `!=` 比较即可。
- 理解 `unreachable_sentinel_t`：`operator==` 永远返回 `false`，
  使无界序列在类型系统中合法表达。
- 理解 `sized_range` 要求 `ranges::size(r)` 是 O(1)；
  `common_range` 要求 `begin()` 与 `end()` 是相同类型。

## 预计练习方向

- 构造有界 `views::iota(0, 10)` 和无界 `views::iota(0)`，分别用 `static_assert` 验证
  `sized_range` / `common_range` / `random_access_range` 的成立情况。
- 对无界 iota 验证 `end()` 类型是 `std::unreachable_sentinel_t`。
- 检查无界 iota 迭代器的 `iterator_concept`（应为 `random_access_iterator_tag`）。
- 用 `views::take(5)` 把无界 iota 变为有界，验证产出的 view 满足 `sized_range`，
  并用 for-range 消费（证明 take 返回的是视图蓝图，不是容器）。
- 构造 `std::ranges::subrange`，验证其为 `sized_range` + `common_range`。

## 进阶预计方向

- 用 `views::take_while` 模拟自定义终止条件，验证结果不满足 `common_range`
  （end 类型是谓词 sentinel，不是 iterator）。
- 对比 `views::iota(0) | views::take(10)` 与 `views::iota(0, 10)` 在
  `sized_range` / `common_range` 上的属性差异。
- 思考：为什么无界 iota 满足 `random_access_range`（iterator 支持算术运算），
  但不满足 `sized_range`（无法 O(1) 计算大小）？

## 验收点

- 三组 `static_assert`（有界 / 无界 / subrange）全部编译通过，无需注释任何一行。
- 能解释为何无界 `iota_view` 仍满足 `random_access_range`，
  但不满足 `sized_range` 和 `common_range`。
- 能说出 `views::take_while` 的 end 类型是什么，以及为什么它不是 `unreachable_sentinel_t`。
- 能区分 `subrange`（包装已有 iterator pair）和 `iota_view`（按需生成值工厂）的设计意图。

## 观察点

- 无界 `iota_view` 的合法性完全依赖 sentinel 分离：end 是 `unreachable_sentinel_t`，
  不持有任何状态，`operator==` 永远返回 `false`，类型系统允许"序列没有终止"的表达。
- `views::take(n)` 不拷贝数据，只创建新的 view，其 end 变成"已读取 n 个元素"的计数 sentinel。
- 有界 `iota_view<int,int>` 的 `size()` 实现是纯算术差值，构造是严格 O(1)，
  符合三条 view 语义公理。
- `subrange` 与 `iota_view` 的区别：前者包装已有 iterator pair，不生成值；
  后者是按需生成整数的值工厂。

## 常见坑

- 混淆"无界"与"无限大小"：无界 iota 的 begin 永远不遇到 end，
  但整数溢出之后行为是未定义的——序列实际上有自然上界。
- 期待无界 `iota_view` 满足 `sized_range`：这不可能，没有已知终止点就无法 O(1) 求大小。
- 用 `ranges::distance` 对无界 iota 求长度：对非 sized_range，`distance` 逐步推进 iterator，
  导致无限循环（或整数溢出）。
- 忘记 `take` 返回的是 view（蓝图），不是容器：`first5` 没有存储任何元素，
  直到 for-range 真正推进迭代器时才开始产出值。
- 把 `views::iota(0, 10)` 和 `views::iota(0) | views::take(10)` 视为完全等价：
  前者是 `common_range`（begin/end 同类型），后者通常不是。

## 提示

- 检查 `iterator_concept`：
  ```cpp
  using It = std::ranges::iterator_t<decltype(unbounded)>;
  static_assert(std::same_as<typename It::iterator_concept,
                              std::random_access_iterator_tag>);
  ```
- 验证 end 类型：
  ```cpp
  static_assert(std::same_as<decltype(unbounded.end()),
                              std::unreachable_sentinel_t>);
  ```
- 不确定某个 range 是否满足某 concept，优先用 `static_assert` 验证，
  不要靠直觉——编译器的错误信息会指出确切的不满足原因。

## 复盘问题

1. `iota_view<int,int>` 和 `iota_view<int,unreachable_sentinel_t>` 的主要区别
   体现在哪几个 concept 的成立情况上？
2. `views::take(n)` 在接受有界 `sized_range` 输入与无界输入时，
   产出 view 的 `sized_range` 属性是否一样？为什么？
3. 如果把无界 `iota_view` 传给需要 `sized_range` 的操作，会发生什么？
4. `unreachable_sentinel_t` 的 `operator==` 返回 `false` 是如何在类型层面
   保证"序列不终止"的？

## 对应官方参考

- P0896R4：C++20 ranges 基础合入，包含 `iota_view` 的初始设计
- cppreference：[`std::ranges::iota_view`](https://en.cppreference.com/w/cpp/ranges/iota_view)
- cppreference：[`std::unreachable_sentinel_t`](https://en.cppreference.com/w/cpp/iterator/unreachable_sentinel_t)
- cppreference：[`std::ranges::subrange`](https://en.cppreference.com/w/cpp/ranges/subrange)
- cppreference：[`std::ranges::sized_range`](https://en.cppreference.com/w/cpp/ranges/sized_range)
- cppreference：[`std::ranges::common_range`](https://en.cppreference.com/w/cpp/ranges/common_range)
