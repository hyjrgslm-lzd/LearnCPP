# 练习 G-3：retry 组合器

## 目标

用已有的基础原语（如 `let_error`）组合出一个更高层的 `retry` 算法，体验"从原语到策略"的抽象跃升。理解为什么 sender-receiver 框架的组合性使得用户可以在框架之外构建复杂控制流。

## 前置理解

- 你理解 `let_error` 的语义：当 sender 以 error 完成时，调用一个函数产生新的 sender 继续执行。
- sender 是惰性蓝图，每次 `connect + start` 都是一次新的执行实例。
- retry 的核心难点不在于逻辑本身，而在于"如何在 sender 框架内表达重复执行"。

## 必做任务

1. **实现 `retry(sender, max_attempts)` 函数**，返回一个新的 sender。用 `let_error` 拦截错误，在错误处理函数中决定是重试还是放弃。

2. **处理计数器的生命周期**：使用 `std::shared_ptr<int>` 或将计数器嵌入 sender 自身。

3. **在每次重试时打印日志**：记录当前第几次尝试和捕获到的错误信息。

4. **构造一个可控的测试 sender**：前 N 次以 error 完成，第 N+1 次以 value 完成。

5. **验证 retry 正常工作**：sender 失败若干次后成功，retry 能恢复。

6. **验证 retry 耗尽后转发错误**：sender 始终失败且超过 max_attempts 时正确转发错误。

## 进阶任务

- **stop_token 取消支持**：每次重试前检查 stop_token，如已停止则以 stopped 完成。
- **指数退避**：第 1 次等 100ms，第 2 次等 200ms，第 3 次等 400ms...（上限 5 秒）。
- retry policy 模式：接受策略对象决定是否重试、等待多久。
- `retry_when(sender, predicate)` 变体。

## 验收点

- retry 在失败次数 < max_attempts 时能成功恢复。
- retry 在始终失败且超过 max_attempts 时正确转发最后一次错误。
- 每次重试都有可观察的日志输出。
- 你能解释 retry 内部的 sender 图结构。
- retry 是纯用户层组合，不需要修改框架内部。

## 观察点

- retry 的核心困难是"在惰性框架中表达重复"。你需要用 `let_error` 把"下一次尝试"表达为一个新的 sender 蓝图。
- 计数器的生命周期是最容易出错的地方。
- retry 是"面向组合"设计哲学的典型产物：框架提供原语，用户在框架外部组合出策略。

## 对应官方参考

- `stdexec` 中 `retry` 相关示例或测试（如有）
- P2300R10 中 `let_error` 的语义说明
- P2175R0 / P2519R0 中对 sender algorithm 组合性的讨论
