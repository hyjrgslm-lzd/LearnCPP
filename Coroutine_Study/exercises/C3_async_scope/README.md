# 练习 C-3：scope 与生命周期收束

先读 [模块 C 的 scope 章节](../../04-模块C-取消与组合.md#c3)。本题先使用共享 `coroutine_study::task_scope` 的参考语义；自写完整 `async_scope` 留到模块 G/Capstone5。

## Part 1：spawn 与 join

打开 [main.cpp](main.cpp)，启动 5 个不同延迟的 `lazy_task<void>`。每个 task 完成时递增计数或打印日志。

scope 的语义是：

```text
scope.spawn(task0)
scope.spawn(task1)
...
scope.join()
  -> 等所有 spawned task 完成
```

`join()` 是可见的收束点。离开 scope 时析构兜底也会请求停止并 join。

## Part 2：异常归属

参考实现 [solution.cpp](solution.cpp) 还启动一个会抛异常的子 task。`task_scope` 保存第一个子任务异常，join 后重新抛出。这样 fire-and-forget 的任务仍有错误归属，不会静默丢失。

## Part 3：悬挂引用时间线

默认不要运行 UB。只画时间线：

```text
局部 std::string msg 创建
spawn task 捕获 msg&
msg 离开作用域并析构
task 后来恢复并读取 msg&
```

scope 能等齐任务，但不能修复错误捕获。需要跨挂起点使用的数据，要么活得比 task 久，要么按值移动进协程帧。

## Part 4：detach 对照

写出所有权对比：

- `scope.spawn(task)`：scope 是 owner，join/drain 点可见，异常和取消有归属。
- `detach(task)`：没有可见 owner，没有必经 join 点，shutdown 时不知道 task 是否还在访问资源。

## 验收

- 5 个 task 都在 scope 退出前完成。

  **答案解析：** 5 个 task 通过 `scope.spawn(...)` 交给同一个 scope，`scope.join()` 是等待全部完成的收束点。即使每个 task 延迟不同，join 返回时计数应已经达到 5。这个结果证明动态任务没有脱离上层 owner。

- 子 task 异常能通过 join 回到调用侧。

  **答案解析：** 子 task 抛出的异常由 `task_scope` 保存为第一个异常，并在 `join()` 后重新抛出。调用侧围绕 join 捕获异常，就能知道后台任务失败原因。这样 fire-and-forget 风格的启动仍然有明确错误归属。

- 你能区分“scope 收束任务”和“数据引用必须有效”。

  **答案解析：** scope 管的是 task 生命周期：启动、等待、取消、异常归属。它不改变被 task 捕获的数据生命周期；如果 task 捕获 `std::string& msg`，而 `msg` 在 task 恢复前已经析构，scope 也只能等到一个会读悬挂引用的 task。正确做法是让数据 owner 活得更久，或按值移动进协程帧。

- 你能说明 detach 断开了哪条所有权链。

  **答案解析：** `detach(task)` 后，上层代码没有可见 owner 持有这个 task，也没有必经 `join()` 点等待它完成。异常没有统一回传位置，取消也没有可靠收束点；如果后台任务还访问资源，shutdown 时调用侧无法知道是否安全。断开的就是“创建者 -> scope owner -> join/drain”的任务生命周期链。
