# 10 模块 H：协程与 sender/receiver 桥接

本模块回答一个工程问题：已经有 sender/receiver 组合子，为什么还需要协程；已经有协程语法，为什么还需要 sender/receiver？

答案是分工不同：

- sender/receiver 描述异步任务、组合关系、调度器、停止、环境传播和 completion signatures。
- coroutine 描述局部控制流，让异步代码按顺序写。
- bridge 把二者接起来：sender 的 `set_value/set_error/set_stopped` 变成协程的恢复、返回、抛出或停止。

做完 H1-H3 后，你应该能闭环回答：是什么，为什么，怎么实现，怎么用。

## 标准边界

C++26 的 execution/task 仍处在工作草案与实现追赶阶段。本文按这些语义讲：

- P2300 sender/receiver：`connect(sender, receiver)` 产生 operation state，`start(op)` 启动，完成时只调用一个 completion channel。
- working draft `[exec.as.awaitable]`：把满足约束的 sender 适配为 awaitable，使协程体能 `co_await sender`。
- P3552 `[exec.task]` 方向：task 既是协程返回类型，也是 sender，可进入 `when_all/then/sync_wait` 等组合。

仓库 reference 使用 pinned NVIDIA/stdexec `nvhpc-26.05`：

- `stdexec::just/just_error/just_stopped/sync_wait/when_all/starts_on/schedule`
- `stdexec::task` / `exec::task`
- `exec::static_thread_pool`

注意：这些 task 是 stdexec 的兼容实现，不是标准库里的 `std::execution::task`。H2 的 `H2_std_task_probe` 由 CMake 真实编译 `std::execution::task<void>` 后生成结果，用来观察当前标准库是否已经提供标准 task。对不支持 pinned stdexec task reference 的工具链，CMake 不创建 reference target，也不注册 CTest。

## H-1：sender -> awaitable

目标：亲手实现 `as_awaitable(sender)`。

核心对象：

- `sender_awaitable` 持有 sender、结果 storage、operation state。
- `bridge_receiver` 持有协程 handle 和 storage 指针。
- `await_suspend` 里 `connect + start`。
- receiver 收到 completion 后写入 storage，再恢复协程。
- `await_resume` 读取 storage：value 返回，error rethrow，stopped 转换成显式异常。

关键正确性：

- operation state 不能是 `await_suspend` 局部变量；它必须活到 sender 完成。
- `set_value/set_error/set_stopped` 三条 channel 都要覆盖。
- sender 可以在 `start()` 内同步完成。此时 receiver 不能直接重入恢复尚未完成 `await_suspend` 的协程。reference 使用一个原子握手：
  - completion 先把结果写进 storage，再把状态改成 completed；
  - 如果协程已经真正挂起，则 resume；
  - 如果 completion 发生在 `await_suspend` 返回前，`await_suspend` 返回 `false`，让当前协程不挂起而继续执行。

练习验收：

- value：`co_await stdexec::just(42)` 返回 42。
- error：`co_await stdexec::just_error(exception_ptr)` 在 `await_resume` rethrow。
- stopped：`co_await stdexec::just_stopped()` 进入 stopped 路径。
- 能画出链路：`co_await sender -> await_transform -> sender_awaitable -> connect -> start -> set_xxx -> await_resume`。

## H-2：`std::execution::task` / `exec::task`

目标：理解标准 task 的语义，不把它当成“更复杂的 lazy_task”。

task 的增量：

- 它是 lazy 的：被连接/等待后才启动。
- 它是 sender：有 completion signatures，能被 `sync_wait/when_all/then` 消费。
- 它有 environment propagation：协程 promise 能把 scheduler、stop token 等环境暴露给内部 `co_await` 的 sender。
- 它有 scheduler affinity：协程恢复应回到关联 scheduler，减少“回调在哪个线程执行”的隐式猜测。

reference 做两件事：

- CMake probe 用 `check_cxx_source_compiles` 真实例化 `std::execution::task<void>`，不只看 feature-test macro。
- stdexec task reference 在支持工具链上真实运行；不支持工具链不建 reference target、不注册 CTest，避免把不可运行路径伪装成通过。

当前 reference 已覆盖：

- `stdexec::task<int>` 体内 `co_await` sender，并作为 sender 被 `sync_wait` 消费。
- `exec::static_thread_pool` + `starts_on`，观察 worker 线程与 main 线程不同。
- `when_all` 汇合两个调度到 worker 的 sender。
- `just_stopped` 通过 `stopped_as_optional` 转成空 `std::optional`。
- `get_stop_token` 从 task environment 读到可用 stop token。

练习验收：

- 能解释 `get_env()` 为什么是协程到 sender 环境传播的入口。
- 能解释 scheduler affinity 保证的是恢复资源，不是魔法并行。
- 能区分标准草案里的 `std::execution::task` 和当前 stdexec 的 `exec::task`。

## H-3：task 同时是 sender 和 awaitable

目标：实现最小 `my_task<T>`，同时满足两条消费路径。

sender 路径：

```text
connect(my_task, receiver)
  -> op_state owns my_task + receiver
  -> start(op_state)
  -> coroutine resumes
  -> final_suspend calls set_value/set_error/set_stopped(receiver)
```

awaitable 路径：

```text
outer coroutine
  -> co_await my_task
  -> await_suspend saves caller as continuation
  -> symmetric transfer to child task
  -> child final_suspend returns continuation
  -> outer resumes and await_resume reads result
```

协程体内等待 sender：

```text
my_task body
  -> co_await stdexec::just(42)
  -> promise.await_transform(sender)
  -> H-1 sender_awaitable bridge
```

关键正确性：

- `external_receiver` 和 `continuation` 是两套不同出口。前者服务 sender 消费者，后者服务协程等待者。
- `final_suspend` 必须用精确 `std::coroutine_handle<promise_type>`。不要用 `from_address` 把派生 promise 伪装成基类 promise；这不是标准保证的对象模型。
- `completion_signatures` 是类型级合同。声明 `set_value_t(T)`，实际就必须沿 sender 路径发出 T。
- 在 pinned stdexec 里，自定义 sender 的 opt-in 是 `using sender_concept = stdexec::sender_tag`，不是旧示例里常见的 `sender_t`。
- 如果一个教学 `my_task` 同时公开 awaiter 三件套，stdexec 可能优先把它当普通 awaitable 适配，绕开自定义 `connect`。reference 的做法是：`my_task` 作为 sender 公开 `connect`；作为 awaitable 时只由自己的 `promise_type::await_transform(my_task&&)` 返回专用 awaiter。

练习验收：

- `connect/start(sender_side())` 返回 42。
- `connect/start(outer())` 中 `outer` 能 `co_await inner()`，返回 20。
- `connect/start(bridge_side())` 中 `my_task` 能 `co_await stdexec::just(42)`，返回 43。

## 构建

```powershell
cmake -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build-h -G Ninja -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build Coroutine_Study/exercises/build-h --target H1_as_awaitable_reference H2_std_task_probe H2_std_execution_task_reference H3_bidirectional_bridge_reference
ctest --test-dir Coroutine_Study/exercises/build-h -R "H[123].*_reference" --output-on-failure
```

## 工程作用

现代 C++ 项目里，协程不是“语法玩具”。它常出现在：

- RPC/client/server：把请求链写成顺序控制流，底层仍由 IO scheduler 驱动。
- 网络与存储 IO：一个 task 表示一次可组合异步操作。
- 结构化并发：`when_all/let_value/sync_wait` 组合 task，统一错误和取消。
- GUI/game/server 主循环：scheduler affinity 控制恢复线程，避免到处手写锁和回调。

判断该不该用协程：如果你需要保留局部状态、顺序表达多个异步步骤、并且要和 scheduler/cancellation/组合子协作，协程 + sender/receiver 是合适模型。如果只是一次简单回调，普通函数或现有 future 可能更便宜。
