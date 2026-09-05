# 练习 J-3：协程调试与 tracing

对应文档：`12-模块J-陷阱诊断与跨编译器.md` §J-3

## 目标

用 GDB 14+ / MSVC Parallel Stacks 调试挂起的协程；实现一个轻量
`traced_awaitable<Inner>` 包装器，在 `await_suspend` / `await_resume`
时自动打 trace。建立"协程调试有章可循"的信心。

## 必做任务

1. 在 MSVC（Visual Studio 2022+）中创建包含 3 个嵌套协程调用的程序：
   `task_c() -> task_b() -> task_a()`，在 `task_a` 内 `co_await` 一个永不
   ready 的 awaitable，调试器暂停后用 Parallel Stacks "Tasks" 视图观察。
2. 在 GDB 14+ / WSL 上验证 `info coroutines` 命令。
3. 实现 `traced_awaitable<Inner>`：
   ```cpp
   template <typename Inner>
   struct traced_awaitable {
       Inner inner_;
       const char* name_;
       void* frame_addr_ = nullptr;
       bool await_ready();
       auto await_suspend(std::coroutine_handle<> h);
       decltype(auto) await_resume();
   };
   ```
   `log()` 应记录：时间戳、awaitable 名称、协程帧地址、当前线程 ID。
4. 把它套在至少 3 个 co_await 点上，运行并输出 trace 日志。
5. 进阶：实现 `traced_task<T>`，在 `initial_suspend` / `final_suspend`
   也打 trace；实现全局协程注册表（key = 帧地址，输出只显示 opaque id）。
6. 用 trace 日志回答：
   - 同时挂起的协程数量；
   - 哪些 co_await 点最长；
   - 是否存在跨线程恢复的协程。

## Starter / Reference

- `main.cpp` 是 starter：展示 `traced_awaitable` 形状和三层调用链。
- `solution.cpp` 是参考答案：实现线程安全 registry、`traced_task<T>`、3 个 `co_await` trace 点，并在结束时断言 registry 为空。
- `COROUTINE_STUDY_BUILD_REFERENCE=ON` 时会生成 `J3_coroutine_tracing_reference` 并加入 CTest。

## 验收点

- 至少在 MSVC 或 GDB 中观察过一次完整的协程逻辑调用链；
- `traced_awaitable` 输出清晰的 `SUSPEND` / `RESUME` 日志，包含协程标识 +
  线程 ID + 帧地址；
- 能解释为什么协程调试需要同时观察"线程栈 + 协程链"两层；
- 知道 MSVC Parallel Stacks "Tasks" 视图依赖编译器生成的元数据。

## 约束

- 不在 tracing wrapper 的内存分配路径上打 trace（避免无限递归）；
- 注册表必须线程安全；
- Release 构建中通过宏关闭 trace（避免性能影响）；
- `frame_addr_` 仅作为 registry key，不要解引用；日志中显示 `coro#N` 这种 opaque id，而不是鼓励读帧内存。

## 提示

- trace 开销必须以本机编译器、日志后端、优化级别实测；不要复用别人的 ns 数字；
- `__PRETTY_FUNCTION__` / `__FUNCSIG__` 可在 Debug 模式下自动生成 awaitable 名；
- 注册表在协程析构后必须 unregister，否则会发生 use-after-free。
