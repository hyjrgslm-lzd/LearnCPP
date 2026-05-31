# 练习 E-2：await_suspend 三种返回值

## 目标

完整实现并对比 `await_suspend` 的三种合法返回类型——`void`、`bool`、`std::coroutine_handle<>`，
通过控制流差异建立"symmetric transfer 是嵌套协程的最优方案"的直觉。

## 必做任务

1. 实现 `awaiter_void`：返回 void，保存 handle，外部手动 resume。
2. 实现 `awaiter_bool_true`：返回 true，行为与 void 一致。
3. 实现 `awaiter_bool_false`：返回 false，不挂起，await_resume 立即被调用。
4. 实现 `awaiter_symmetric`：返回 `coroutine_handle<>`，框架直接跳转到目标。
5. 写两个相互配合的协程 `coro_A / coro_B`，观察 symmetric transfer 控制流。

## 验收点

- 通过日志看到三种返回值的不同控制流。
- 你能解释 void 模式下"必须有外部代码 resume"的含义。
- 你能解释 bool=false 与 await_ready=true 的语义差异（前者已经决定挂起又反悔）。
- 你能说明 symmetric transfer 不是"A 调 B"，而是"框架调 B、A 帧已退栈"。

## 提示

- 每种返回值用一个独立 awaiter 类型 + 独立协程，避免控制流混淆。
- 注意：`bool=true` = 挂起（同 void），`bool=false` = 不挂起（立即继续）。务必对照 cppreference。
- symmetric transfer 实验需要先创建 B 拿到 handle 再传给 A 的 awaiter。
