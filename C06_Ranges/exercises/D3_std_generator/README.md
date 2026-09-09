# 练习 D-3：std::generator 协程桥

> 对应章节：../../06-模块D-C++23高阶视图与协程桥.md §练习 D-3
> 提案：P2502R2 (std::generator, elements_of)
> C++ 标准：C++23

## 目标

把协程的惰性产值接入 ranges 管道：理解 `std::generator`（P2502R2）如何同时满足 `input_range` 与 `view`，如何与 `views::take`、`ranges::to`、管道组合；理解 `elements_of` 的委托产值语义；确认 `generator` 是单遍（not forward）的根本原因。

## 前置理解

- 已了解协程基本语法：`co_yield`、`co_return`；知道 `std::generator` 在 `<generator>` 头文件中，C++23 引入。
- 接受 `std::generator<T>` 是 `input_range + view`，但不是 `forward_range`——它是单遍的，迭代器是 move-only 的 `input_iterator`。
- 已读 `01-心智模型.md` 中 view 的三条语义公理（O(1) move / O(1) destroy / O(1) copy 或不可 copy）。

## 必做任务方向

1. **斐波那契生成器**：实现 `std::generator<int> fib()`，用 `views::take(10)` 截取前 10 个，用 `ranges::to<vector<int>>()` 收集。
2. **concept 验证**：用 `static_assert` 验证 `input_range` 和 `view` 成立，`forward_range` 不成立，并说出原因。
3. **move-only 验证**：验证 `std::copyable` 不成立，`std::movable` 成立。
4. **单遍语义演示**：手动用 `it/end` 推进，先取 5 个再取接续的 5 个，证明不能从头重播。
5. **树的递归遍历**：用 `co_yield std::ranges::elements_of(walk_tree(child))` 实现前序 DFS，验证输出 `1 2 4 5 3`。
6. **接入管道**：`walk_tree(tree) | views::filter([](int v){ return v >= 3; })`，验证产出 `3 4 5`。

## 进阶预计方向

1. 实现 `flatten` 演示 `elements_of` 委托语义，对比等价的显式 for 循环写法。
2. 对比 `views::repeat + transform`（有限表达）与 `generator`（有状态机）的能力边界。
3. 把 `walk_tree` 改为后序遍历，观察 `elements_of` 与 `co_yield` 嵌套的写法差异。
4. 验证 move 后继续使用原 generator 的行为（通常协程句柄变为 null，继续迭代是 UB）。

## 验收点

- 斐波那契生成器代码编译通过，`fib() | views::take(10) | ranges::to<vector>()` 得到正确结果。
- 用 `static_assert` 证明 `std::generator<int>` 满足 `input_range` 和 `view`，但不满足 `forward_range`，并能说出原因（迭代器 move-only，无 multi-pass 保证）。
- 能说清楚 `co_yield std::ranges::elements_of(range)` 的语义：把另一个 range 的所有值委托给当前 generator 产出。
- 能说出 generator 协程里为何不能混 `co_await`（promise_type 不提供 `await_transform`）。
- 用运行结果证明两次 `take(5)` 消费的是序列的不同部分（第 1-5 和第 6-10）。

## 观察点

- `std::generator<T>` 的"惰性"来自 `initial_suspend()` 返回 `std::suspend_always`：协程体不会在 `fib()` 调用时立即执行，而是等到第一次 `++it` 时才运行到第一个 `co_yield`。与 `views::iota` / `views::filter` 的管道惰性是同一层的抽象。
- `std::generator` 满足 O(1) move（转移 `coroutine_handle`，只是一个指针大小的操作）、O(1) destroy（`coroutine_handle.destroy()` 清理协程帧，不遍历已产出的元素）。不满足 copyable，因为协程状态不能被复制。
- `elements_of` 是 P2502R2 引入的协程扩展点，允许 generator 递归委托。实现上可以利用对称转移（symmetric transfer）在协程之间直接切换，减少开销。与 `join_view` 展平嵌套的思路类似，但发生在协程层面。
- 单遍不仅是运行时约束，也是类型系统约束：`input_iterator` 不提供 multi-pass 保证，`std::sort`、`std::distance`（非 sized）等算法无法与 generator 配合。

## 常见坑

- **把 generator 当 forward_range**：`std::ranges::distance(g)` 对非 sized input_range 会遍历整个序列（对无界 generator 会无限循环）；`std::ranges::sort` 要求 random_access，generator 完全不满足。
- **在 generator 协程里混 co_await**：`std::generator` 的 promise_type 不提供 `await_transform`，因此 `co_await some_future` 不会编译。如果需要异步 generator，需要第三方库（如 cppcoro）。
- **多次迭代同一个 generator 期待从头开始**：generator 的 coroutine_handle 只有一个，执行位置只能向前推进；第二次从上次末尾继续，不会自动重置。要"重新播放"只能重新调用函数创建新的 generator 对象。
- **move 后继续使用原 generator**：`auto g2 = std::move(g1)` 之后，`g1` 处于移后状态（协程句柄通常变为 null），继续迭代 `g1` 是未定义行为。
- **把 elements_of 当普通函数**：`elements_of` 只能在 generator 协程体内用 `co_yield` 委托，不能在协程外部使用。

## 提示

- 测试"单遍"属性：先调用 `g.begin()` 推进几步，再调用 `g.begin()` 注意它返回的是同一个迭代器（已在当前位置），不是"从头开始的新迭代器"——与 `iota_view::begin()` 总是返回起始位置的迭代器形成对比。
- 如果 `<generator>` 头文件还不可用，检查编译器版本（MSVC 17.5+ / GCC 13+ / Clang 17+）和 C++23 标志。
- 递归 generator（`walk_tree`）用普通 `co_yield` 实现时，每次递归调用 `walk_tree(child)` 都创建新的 generator，整体是 O(树深) 个协程帧同时活跃；用 `elements_of` 版本在支持时减少帧切换次数。

## 复盘问题

1. `std::generator<T>` 和 `views::iota(0)` 都是"无界序列"，它们的底层机制有什么本质差异？（提示：iota 是状态很少的计算，generator 是任意复杂的协程执行）
2. 为什么 `std::generator` 满足 view 的三条语义公理，但不满足 copyable？
3. `co_yield std::ranges::elements_of(other)` 和 `for (auto v : other) co_yield v` 在语义上等价，为什么前者可能更高效？（提示：symmetric transfer / 协程帧切换次数）
4. 如果你的系统需要"可以被多次独立消费的惰性序列"，`std::generator` 不适合，应该用什么代替？
5. 对比 `std::generator` 与 `views::filter | views::transform` 管道：什么时候用协程表达更清晰，什么时候用管道更清晰？

## 对应官方参考

- P2502R2：`std::generator`（C++23 协程 generator，input_range + view，elements_of 委托）
- P2474R2：`views::repeat`（模块 A 已讲，在进阶对比部分引用）
- cppreference：[`std::generator`](https://en.cppreference.com/w/cpp/coroutine/generator)
- cppreference：[`std::ranges::elements_of`](https://en.cppreference.com/w/cpp/ranges/elements_of)
