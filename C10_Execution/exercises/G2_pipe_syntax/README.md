# 练习 G-2：pipe 语法

## 目标

为你的 sender adaptor 添加管道语法支持，让 sender 链可以写成 `just(1) | my_then(f) | my_then(g)` 的线性形式。理解 closure object、partial application 和 `operator|` 在类型层面的协作。

## 前置理解

- 你已经在 G-1 中实现了 `my_then(sender, f)`，它接受两个参数。
- 管道语法的核心思路是把双参数调用拆成两步：先传 `f` 得到一个 closure，再通过 `operator|` 把 sender 喂进去。
- `ranges::views` 的管道语法是相似的设计先例。
- 链式管道需要 closure 之间也能用 `operator|` 组合。

## 必做任务

1. **实现 `my_then(f)` 单参数重载**，返回一个 `my_then_closure<F>` 对象。

2. **实现 `operator|(Sender, my_then_closure<F>)`**：左操作数是 sender，右操作数是 closure，返回 `my_then_sender`。

3. **验证单层管道**：`just(1) | my_then(f)` 返回正确结果。

4. **验证链式管道**：`just(1) | my_then(f) | my_then(g)` 返回正确结果。

## 进阶任务

- **实现 `sender_adaptor_closure` 基类**，使 closure 之间也能用 `|` 组合。
- 让其他 adaptor（如 `tap`）也支持管道语法。
- 研究 `std::ranges::range_adaptor_closure` 的设计。
- 尝试让 `sync_wait` 也能出现在管道末尾。

## 验收点

- `just(1) | my_then(f) | my_then(g) | sync_wait` 能编译通过并返回正确结果。
- 你能画出 `my_then(f)` 返回的 closure 对象的类型结构。
- 你能解释 `operator|` 的重载决议为什么能区分不同情况。
- 管道语法没有引入额外的运行时开销，只是改变了表达形式。

## 观察点

- 管道语法的本质是 partial application + operator overloading。
- 链式管道之所以能工作，是因为每个 `|` 的结果仍然是 sender，可以继续被下一个 `|` 消费。
- `sender_adaptor_closure` 基类让你不需要为每个 adaptor 单独写 `operator|`，这是框架级的复用手段。
- 管道语法让数据从左到右流动，与人类阅读习惯一致。

## 对应官方参考

- P2300R10 中对管道语法的说明
- `stdexec` 源码中 `__sender_adaptor_closure` 相关实现
- C++23 `std::ranges::range_adaptor_closure` 的设计文档
