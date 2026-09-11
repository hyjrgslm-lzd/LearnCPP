# C2_10 scope 与生命周期

本题讲 scope 的核心责任：接受异步工作、拥有 operation state、等待已接受工作完成，再释放父作用域。`exec::async_scope` 是 stdexec 扩展；标准 task/counting scope 的接口由后续 `10-task-and-scope.md` 主讲。

## 知识

sender 是描述，不拥有一次执行。`operation_state` 才是 connect 后的执行实例。启动后如果没有 scope 或 future 持有它，就很难证明它何时结束，也无法安全销毁执行资源。

`scope.on_empty()` 代表 rendezvous 点：不再有已接受工作存活。关闭后拒绝新工作、已接受工作 drain，是结构化并发的底线。

## 机制

可运行检查直接使用 `exec::async_scope` 验证同一组不变量：`spawn` 接受 3 个 fire-and-forget 工作；`on_empty()` 等三个工作都完成后才标记可释放；`spawn_future` 的结果汇总为 30；子任务错误 `"child failed"` 被保存。

bad 版本只完成 1 个已接受工作就报告释放，检查器拒绝。

## 必做

1. 区分 sender、operation state、scope、执行资源。
2. 记录 accepted work 数量。
3. 等全部 accepted work 完成后再释放父作用域。
4. 保存 result-returning work 的结果。
5. 保存 error path，不能只统计成功。

## 进阶

- 对比 `scope.spawn(...)` 与 `scope.spawn_future(...)`：前者只交给 scope 拥有，后者还给调用方一个可等待的 future sender。
- 对比 `start_detached`：它没有调用者可等待的生命周期边界。

## 答案解释

Reference 使用真实 `exec::async_scope`、`exec::static_thread_pool`、`spawn`、`spawn_future` 与 `on_empty()`。核心不变量是：父 scope 收束之前，子 operation state 不能释放；错误不会被成功计数吞掉。
