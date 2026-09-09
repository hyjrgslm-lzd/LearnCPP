# H-2 std::execution::task / stdexec task

知识讲解：[H2 对应章节](../../10-模块H-协程与sender_receiver桥接.md#h2)。

对应主讲义：`10-模块H-协程与sender_receiver桥接.md` 的 H-2。

这题的重点是观察 task 作为 sender 的工程语义。当前工作草案里的 `[exec.task]` 仍在演进；本仓库用 pinned NVIDIA/stdexec `nvhpc-26.05` 的 `stdexec::task` 跑同类行为。`H2_std_task_probe` 另用 CMake 真实编译 `std::execution::task<void>`，确认当前标准库支持状态。

reference 覆盖五个观察点：

- `stdexec::task<int>` 体内能 `co_await` sender，外部能被 `sync_wait` 消费。
  **答案解析：** 这里观察的是 pinned NVIDIA/stdexec `stdexec::task`，不是已发布标准库里的固定类型。task 创建后保持 lazy，`sync_wait` 连接并启动它；协程体内的 `co_await sender` 通过 task promise 的 awaitable 支持接入 sender completion。
- `exec::static_thread_pool` 配合 `starts_on(just | then)` 产生可观察线程切换。
  **答案解析：** `starts_on(pool.get_scheduler(), sender)` 让输入 sender 在 pool 资源上启动。reference 的输入 sender 内部没有再切换资源，所以 `then` 里记录的线程不同于 main；这只能说明这条组合链的启动资源，不代表所有后续 completion 永远固定在同一线程。
- `when_all` 启动两个分支并汇合 value。
  **答案解析：** 两个分支分别算出 21 和 32，`when_all` 等齐后把 value 作为 tuple 交给外层 task。外层 task `co_return left + right`，所以输出 `when_all_sum=53`。
- `just_stopped` 经 `stopped_as_optional` 变成空 `std::optional`。
  **答案解析：** `just_stopped` 走 stopped channel，没有 value。`stopped_as_optional` 把 stopped 显式转成 value channel 的 `nullopt`，所以调用侧可以用 optional 判定“未产生值”。
- `get_stop_token` 能从 task environment 读到 stop token。
  **答案解析：** task promise 连接到 receiver environment 后，`get_stop_token()` 能查询停止通道。`stop_env=1` 说明 stop token 是 possible；是否已经请求停止、子 sender 是否响应，还要看具体 token 状态和检查点。

预测输出里的 `when_all_sum=53` 来自两个 worker 分支：20 加 1 得 21，30 加 2 得 32。`worker_changed=1` 来自本题的 `just | then` 分支经 `starts_on` 从 pool scheduler 启动，分支内部没有再切换资源。`stop_env=1` 说明 task promise 能访问 environment。

运行：

```powershell
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build-h-release -G "Visual Studio 18 2026" -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build C09_Coroutines/exercises/build-h-release --config Release --target H2_std_task_probe H2_std_execution_task_reference
ctest --test-dir C09_Coroutines/exercises/build-h-release -C Release -R H2_std_execution_task_reference --output-on-failure
```

如果使用 MinGW/GCC + Windows，CMake 会构建 starter 和 probe，但可能不会创建 `H2_std_execution_task_reference` target。这只说明 pinned stdexec task reference 在该工具链上未启用。

**答案解析：** `H2_std_task_probe` 是标准库能力探针，负责实际编译 `std::execution::task<void>`；reference target 使用 pinned stdexec task 观察同类 sender/task 语义。两者回答的问题不同：probe 回答当前标准库是否提供标准类型，reference 回答本课程固定依赖下的 task、environment、`starts_on`、`when_all` 行为。
