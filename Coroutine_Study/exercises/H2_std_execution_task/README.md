# H-2 用 std::execution::task<T>（P3552R3）

对应文档：`10-模块H-协程与sender_receiver桥接.md` 「练习 H-2」。

## 目标

使用 P3552R3 定义的 `std::execution::task<T>`，理解 C++26 标准 task 的核心特性：
scheduler affinity、environment propagation、`co_await schedule(sch)` 切换调度器、
以及 `co_await when_all` 的组合语义。

## 编译期守卫

骨架顶部用 `#if defined(__cpp_lib_execution) && __cpp_lib_execution >= 202902L`
检测 P3552 是否可用：
- 可用：`task_ns = std::execution`，使用标准 `std::execution::task`。
- 不可用（当前绝大多数 toolchain）：`task_ns = exec`，使用 stdexec 的 `exec::task`。

两者在 scheduler-aware / completion_signatures / await_transform 上语义等价。

## 必做任务

1. 跑通测试 1：用 `exec::single_thread_context` 创建一个 scheduler，通过
   `starts_on(sch, task)` 把 scheduler 注入 task。验证 `co_await schedule(sch)`
   后线程 ID 切换到 single_thread_context 的工作线程。
2. 跑通测试 2：`co_await when_all(fetch_a(), fetch_b())` 并行汇合两个子 task，
   sync_wait 取出 tuple。
3. 在笔记中画出环境传播链：
   `sync_wait env → connect → task promise env → 子 sender env`。
4. 解释 scheduler affinity 如何让 task 中的局部变量"无锁可读"。

## 进阶任务

- 自实现 `my_scheduler`（类似 stdexec run_loop），让 task 在你的 scheduler 上恢复。
- 为 task 的 `get_env` 添加 stop_token 转发，验证 cancellation 路径。
- 把 task 与 `then` / `let_value` 等 sender 组合子串成一条管道。

## 验收点

- 你能在 task 中用 `co_await schedule(sch)` 切换调度器，并观察线程 ID 变化。
- 你能用 `when_all` 并行组合多个 task，并用 `sync_wait` 消费。
- 你能讲清 `get_env()` 是协程到 sender 环境传播的唯一通道。

## 提示

- 把 scheduler 注入 task 的标准模式是 `starts_on(sch, task)`——不要直接 sync_wait。
- 当前主流 toolchain（GCC 14、Clang 19、MSVC 17.10）尚未 ship P3552 标准 task；
  本练习以 stdexec exec::task 为等价实现。
- 如果 stdexec 不在你的 include path，确认 Agent 1 的 helper `coroutine_study_link_stage3_deps`
  已被调用。
