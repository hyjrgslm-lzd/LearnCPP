# 练习 8：取消不是错误

## 目标

亲手触发取消流程，观察 stop_token 如何在 sender 图中传播，建立"`stopped` 是独立于 `error` 的一等 completion channel"的直觉。

## 前置理解

- 你已经知道 completion 有三条 channel：value、error、stopped。
- 你已经在练习 7 中使用过 `upon_error`。
- 你知道 `stop_token` 概念来自 `<stop_token>` 头文件。
- 你接受这题的重点是观察取消语义，不是实现完整的取消框架。

## 必做任务

1. 创建一个 `in_place_stop_source`，获取对应的 `stop_token`。
2. 构造一组至少 3 个并列的 sender 分支，每个分支内部有一段模拟较长计算的循环。
3. 在每个分支的循环中，周期性检查 `stop_token` 的状态。如果检测到取消请求，提前退出循环并让 sender 完成走 `set_stopped` 通道。
4. 用 `when_all` 汇合这些分支。
5. 在主线程或某个辅助 sender 中，在短暂延迟后调用 `stop_source.request_stop()`。
6. 观察并记录：
   - 哪些分支正常完成（`set_value`）？
   - 哪些分支因取消而停止（`set_stopped`）？
   - `when_all` 的最终 completion 走了哪条 channel？
7. 使用 `upon_stopped` 或 `let_stopped` 把停止路径转换成一个可观察的结果，例如 `CancelledResult`。

## 进阶任务

- 注册一个 `stop_callback`，在取消被请求时打印一条日志，确认回调机制生效。
- 观察 `when_all` 在一个分支出错或停止后，是否向其余分支传播取消。如果你本地版本支持，记录这种级联取消的过程。
- 再做一个版本：故意不检查 stop_token，让分支"无视取消"，观察 `when_all` 在这种情况下如何处理。

## 验收点

- 你能亲手触发取消并观察到 `set_stopped` 被调用。
- 你能区分 `set_stopped` 和 `set_error`：前者是"有意义的终止"，后者是"出错"。
- 你能解释 stop_token 如何通过 environment 传播给 sender 图中的各个阶段。
- 你能说明 `when_all` 在一个分支非正常完成时对其余分支的处理方式。

## 常见坑

- 把取消请求当成"立即杀死任务"，没有意识到它是协作式的。
- 没有在计算循环中检查 stop_token，导致取消请求被完全忽略。
- 把 `stopped` 和 `error` 混为一谈，在 `upon_error` 里处理停止事件。
- 过于关注线程细节，而忽略了取消作为 completion channel 的语义。

## 对应官方参考

- P2300 中 stop_token 在 environment 中的角色说明
- `<stop_token>` 标准库头文件
- `stdexec` 中 `upon_stopped` 的基础用法
