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
   也打 trace；实现全局协程注册表（key = 帧地址）。
6. 用 trace 日志回答：
   - 同时挂起的协程数量；
   - 哪些 co_await 点最长；
   - 是否存在跨线程恢复的协程。

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
- `frame_addr_` 仅作为协程实例标识，不要解引用 —— 帧布局是编译器内部细节。

## 提示

- 单 awaitable 的 trace 开销 ~100-500ns（主要在日志 I/O），帧地址记录几乎零开销；
- `__PRETTY_FUNCTION__` / `__FUNCSIG__` 可在 Debug 模式下自动生成 awaitable 名；
- 注册表在协程析构后必须 unregister，否则会发生 use-after-free。
