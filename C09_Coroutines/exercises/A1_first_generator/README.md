# 练习 A-1：第一个 generator

先读 [模块 A 的 co_yield 章节](../../02-模块A-三关键字与最小协程.md#a1)。如果对 generator 的启动和推进还不稳，回看 [00 的 generator 模型](../../00-预备知识-执行模型与标准库.md#generator-model) 与 [generator 生命周期](../../00-预备知识-执行模型与标准库.md#generator-lifetime)。

本题验证三件事：generator 构造后不执行协程体；迭代器推进一次，生产者运行到下一次 `co_yield`；暂停期间局部状态继续保存在协程状态中。

## Part 1：有限 Fibonacci

打开 [main.cpp](main.cpp)，把 `fibonacci(int n)` 的占位 `0..n-1` 改成真实 Fibonacci 状态机：`a` 保存当前值，`b` 保存下一个值，每次 `co_yield a` 后再更新状态。

运行前先预测：`auto gen = fibonacci(10);` 后，协程体日志不会出现；进入循环并请求第一个元素时，协程才执行到第一个 `co_yield`。

**答案解析：** 预测结论是“构造后没有协程体日志，第一次消费时才有”。generator 的 `initial_suspend` 让函数调用先返回 generator 对象，`a/b/i` 等状态存在协程帧里但还没运行用户代码。range-for 取得迭代器并请求第一个值时恢复协程，协程执行到第一个 `co_yield a` 后再次暂停。

参考实现 [solution.cpp](solution.cpp) 用 `resumed` 断言这一点：构造 generator 后 `resumed == 0`，完整遍历后 `resumed == 1`。

## Part 2：无限序列加 `views::take`

实现 `fibonacci_inf()`，循环条件为 `for (;;)`, 然后用 `std::views::take(10)` 或 `take(15)` 限制消费数量。无限 generator 安全的原因是消费者有边界。

`take` 验证的是“最多交付 N 个元素”。不要用它直接断言底层 generator 只恢复 N 次；某些实现为了判断结束，可能在消费第 N 个值后再尝试推进一次。若要精确观察恢复次数，用有限 generator 日志，或写手动循环：拿到第十个值后立刻 `break`。

## Part 3：逐行文本

实现 `read_lines()`，用 `co_yield std::string{...}` 产出 5 到 8 行。main 端打印行号。观察行号每增加一次，协程体才继续到下一行产出。

做完后画一条时序：构造 generator、第一次 `begin()`、每次 `++it`、每个 `co_yield`、generator 析构。

## 验收

- 有限 Fibonacci 前 10 项正确。

  **答案解析：** 前 10 项应为 `0, 1, 1, 2, 3, 5, 8, 13, 21, 34`。状态更新顺序是先 `co_yield a`，恢复后再计算 `next = a + b; a = b; b = next;`，所以每次交付的是当前 `a`，不是更新后的下一项。

- 无限 Fibonacci 只消费指定前缀。

  **答案解析：** 无限 generator 本身没有终止条件，但它是惰性的，只有消费端推进时才继续执行。`views::take(10)` 给消费端加上最多 10 个元素的边界，所以程序不需要生成完整无限序列。不同实现可能为结束判断多推进一次，验收重点是输出前缀正确且消费有边界。

- `read_lines()` 逐行产出文本。

  **答案解析：** `read_lines()` 每遇到一行就 `co_yield` 一个 `std::string`，消费者解引用迭代器读到当前行。下一行要等消费者执行下一次推进才会产生。当前 Reference 按代码中的产出顺序得到 `alpha`、`beta`、`gamma`、`delta`、`epsilon` 五行。

- 日志能证明“消费者推进 -> 生产者继续”。

  **答案解析：** 构造 generator 后生产者日志不应出现；调用 `begin()` 或第一次进入 range-for 时，生产者才运行到第一个 `co_yield`。每次 main 消费当前值后，下一次迭代推进会让生产者从上一个 `co_yield` 后继续。日志如果呈现 main 与 generator 交替出现，就说明控制权按预期来回切换。
