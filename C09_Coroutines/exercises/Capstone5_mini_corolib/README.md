# 结课项目 5：mini 协程库实现

对应正文：[14 第三阶段结课：mini 协程库实现](../../14-第三阶段结课-mini协程库实现.md)。

这个项目把模块 D 到 G 的机制组合成一个教学 mini 库。核心路径只用 C++ 标准库；stdexec sender 桥接是可选扩展，只有相关 target 可用时才构建。

项目稳定 ID：`Capstone5_mini_corolib`。

## 你要实现的组件

| 组件 | 文件 | 核心能力 |
| --- | --- | --- |
| `mini::task<T>` | `include/mini/task.hpp` | lazy 启动、单 owner、单次结果消费 |
| `mini::generator<T>` | `include/mini/generator.hpp` | 同步惰性序列，input iterator |
| `mini::shared_task<T>` | `include/mini/shared_task.hpp` | 多 awaiter 共享一次 producer 结果 |
| `mini::when_all` | `include/mini/when_all.hpp` | 二元 barrier，全部分支收束后返回 tuple |
| `mini::when_any` | `include/mini/when_any.hpp` | 首个成功 value 获胜，全部分支收束后返回 |
| `mini::sync_wait` | `include/mini/sync_wait.hpp` | final completion 转 condition_variable 唤醒 |
| `mini::async_scope` | `include/mini/async_scope.hpp` | 结构化等待 in-flight task |
| `mini::stop_token` | `include/mini/stop_token.hpp` | 基于标准停止源/令牌的协作取消 |
| `mini::single_thread_executor` | `include/mini/single_thread_executor.hpp` | 队列保存待恢复 handle |
| `mini::as_awaitable` | `include/mini/as_awaitable.hpp` | 可选 sender 到 awaiter 桥接 |

## 实现顺序

先完成 `task<T>` 和 `sync_wait`。它们是后续测试入口。然后写 `generator` 与 `run_loop`，再写 `when_all/when_any`。`shared_task`、`async_scope` 和可选 stdexec bridge 放在后面，因为它们依赖前面的完成通知和生命周期约定。

每一层完成后跑对应测试。协程生命周期错误通常来自上一层协议没有锁住，例如二次启动、完成后重复消费、或 loser 尚未收束时提前析构 operation。

`tests/` 是学生实现检查，不是占位 demo。未完成 TODO 时，相关测试会运行到学生接口并以普通失败结束；补完后同一测试应覆盖 value、void、error、组合收束和 scope 非空 drain；可选 stdexec 桥接还覆盖 stopped 映射异常。`reference/tests/` 独立验证完整答案，可用 `ctest -L reference` 或 `-R mini_reference_` 单独运行。

默认核心验证不注册学生 starter 测试。要检查自己的 TODO，请用 `student` preset，或配置时显式设置 `-DCOROUTINE_STUDY_TEST_STARTERS=ON`。

## 必须保持的契约

`task`：

- 禁 copy，move 转移 owning handle。
- `start()` / `co_await` 后标记为已启动。
- 二次 `start()` 拒绝。
- `await_resume()` 只允许在完成后消费一次。
- 异常由 promise 保存，消费端重新抛出。

`sync_wait`：

- 只接受未启动 root task。
- 只调用一次 `start()`。
- 完成通知来自 `final_suspend` callback。
- 成功返回 `std::optional<std::tuple<T>>` 或 `std::optional<std::tuple<>>`。
- 错误路径重新抛出保存的异常。

`when_all`：

- 两个分支都先启动。
- 每个分支完成时保存自己的结果或首个异常。
- 最后一个分支完成后恢复父协程。
- 任一失败时，全部分支收束后再传播首个异常。

`when_any`：

- 首个成功 value 写入 `std::variant<T1, T2>`。
- winner 产生后调用 `stop_source.request_stop()`。
- loser 仍要完成收束。
- 两个分支都失败时传播首个异常。

`shared_task`：

- producer 只启动一次。
- 结果缓存在 shared state 中。
- 多个 awaiter 都能读到结果拷贝。
- 异常同样缓存在 shared state 中。

## 对象关系

`task`：

```text
task<T>
  -> coroutine_handle<promise_type>
       -> promise { value, error, continuation, completion callback, flags }
```

`when_all`：

```text
when_all_op
  -> left/right child task
  -> left/right runner task<void>
  -> result slots + first_error
  -> remaining count
  -> parent coroutine_handle
```

`sync_wait`：

```text
sync_wait_state
  -> mutex + cv + done
promise.final_suspend
  -> callback(state)
  -> notify waiting thread
```

`as_awaitable` 可选桥接：

```text
await_suspend
  -> connect sender receiver
  -> start operation
  -> atomic phase 处理同步完成
receiver completion
  -> 保存 value/error/stopped
  -> resume caller 或记录已完成
```

## 核心 reference 验证

```powershell
cmake -S C09_Coroutines/exercises --preset verify-core
cmake --build C09_Coroutines/exercises/build/verify-core --config Release --target mini_reference_task mini_reference_generator mini_reference_sync_wait mini_reference_when_all mini_reference_when_any mini_reference_run_loop mini_reference_task_scope mini_reference_stop mini_reference_shared_task
ctest --test-dir C09_Coroutines/exercises/build/verify-core -C Release -R "mini_reference_"
```

可选 stdexec 桥接 target 只在 `stdexec::stdexec` 存在时构建。核心 reference 不应因为缺少 stdexec 而配置失败。

学生 starter 测试只在 `COROUTINE_STUDY_TEST_STARTERS=ON` 时注册为 `starter` 标签，未完成时不标记 `WILL_FAIL`。这能避免“预期失败”被误读成课程 Reference 通过。

## 各测试观察点

### Student Part 与实际检查

| Part | 独立编辑入口 | 检查入口 | 当前起点与解析 |
|---|---|---|---|
| task 基础 | task.hpp | mini_task_test | 已提供 owning task；值、重复 start/consume 拒绝。它不证明 sender TODO 完成。 |
| task sender 扩展 | task.hpp 的 metadata/connect | mini_task_sender_test（stdexec） | 直接消费学生 member connect，排除库自动 awaitable fallback；先读 H3，connect 不启动，start 发一次 value/error。 |
| generator | generator.hpp | mini_generator_test | 迭代观察；frame owner 必须覆盖 iterator 寿命。 |
| sync_wait | sync_wait.hpp | mini_sync_wait_test | 起点抛 TODO；先 start 一次，再等待 final 通知，不能把缺结果当 stopped。 |
| when_all | when_all.hpp | mini_when_all_test | 起点抛 TODO；前置 sync_wait 完成后检查结果和异常，完整并发场景另由 Reference 覆盖。 |
| when_any | when_any.hpp | mini_when_any_test | 二元非 void 契约；两个 manual_event 先登记，右侧先释放必须获胜，同时 request_stop，左侧收束前 root 不返回。 |
| shared_task | shared_task.hpp | mini_shared_task_test | 两个 awaiter 等同一事件，producer 只启动一次；完成后两者及晚到者都读到缓存。 |
| executor | single_thread_executor.hpp | mini_run_loop_test | enqueue/run_one 已提供，schedule 留给学生；入队不能内联恢复，run_one 只恢复一次。阻塞 run 直到 stop，与 Reference 非阻塞 run 的驱动口径分开。 |
| scope | async_scope.hpp | mini_scope_test | 实际消费非空工作，不能只检查空 scope；source 需要有限收束。 |
| stop | stop_token.hpp | mini_stop_test | 标准别名已提供，观察已有/晚到回调及幂等请求；不证明 in-place 无分配扩展。 |
| sender awaitable | as_awaitable.hpp | mini_as_awaitable_test（stdexec） | 检查 value/error/stopped 映射；任务未实现与 stopped 必须区分。 |

这些检查会调用学生操作，未完成项保留普通非零失败，不使用 WILL_FAIL。参考值、void、错误、并发等完整测试仍由下列 Reference 列表承担，不能把参考结果套到 Student。独立可做性验证及行为型反例见 [mini Student 审查](../../references/validation/c09-refresh/reviews/mini-student-review.md)。

首个错误不等于 when_any 完成：该组合保留 first-success 语义，只有成功 winner 请求 stop。没有成功值时，所有输入必须自行完成或响应外部取消；“一个 error 加一个永远等 stop 的输入”本身没有保证有限完成。不能为让这种输入自动退出而悄悄改成 first-error cancel。

- `task_test.cpp`：返回值、重复启动拒绝、重复消费拒绝。
- `generator_test.cpp`：range-for 推进、当前值保存、移动 owner。
- `sync_wait_test.cpp`：值、void、异常、跨线程完成、frame 析构。
- `when_all_test.cpp`：同步完成、延迟完成、并发完成、fail-delay。
- `when_any_test.cpp`：winner、同步完成、loser stop、全部失败。
- `run_loop_test.cpp`：handle 入队后由 loop 恢复。
- `task_scope_test.cpp`：spawn 后等待 in-flight 归零。
- `stop_test.cpp`：停止源和令牌状态传播。
- `shared_task_test.cpp`：一个 producer，多次 await，共享结果。

## 完成标准

完成后你应能从源码解释：

1. `task` 的 frame 谁拥有，谁销毁。

   **答案解析：** `task<T>` 持有 `std::coroutine_handle<promise_type>`，它是 frame 的单 owner。move 只转移 handle，copy 被禁止；析构时若 handle 非空就 `destroy()`。reference 的 `task` 用 started/consumed 标志保护启动和消费，但最终销毁仍归 owner wrapper。
2. `final_suspend` 怎样通知 continuation 或 completion callback。

   **答案解析：** 协程体完成后先把 value/error 存入 promise，再进入 `final_suspend`。reference 的 final awaiter 优先读取并调用 completion callback；没有 callback 时返回 continuation；都没有时返回 `std::noop_coroutine()`。callback、state、continuation 会在发布完成前读出，因为通知后其他线程可能销毁 frame。
3. `sync_wait` 怎样把完成信号转成线程唤醒。

   **答案解析：** `sync_wait` 创建 `sync_wait_state`，把其地址和 `complete` callback 写进 root promise，然后 `start()` 一次并阻塞在 `cv.wait(done)`。root final path 调用 callback，设置 done 并 `notify_one()`。线程醒来后读取 promise 里的 tuple value 或重抛 error。
4. `when_all` 为什么等全部分支完成后才返回。

   **答案解析：** `when_all` 是 barrier：两个 runner 都要写完各自结果或错误，remaining 到 0 后父协程才能恢复。若某分支失败，reference 保存首个 error，但仍等待另一分支收束后再传播。这样 operation 中的结果槽、错误槽和子 task 生命周期都不会提前失效。
5. `when_any` 为什么 winner 产生后仍等待 loser 收束。

   **答案解析：** winner 只表示第一个成功 value 已写入 `std::variant<T1, T2>`；loser 仍可能持有 operation 状态并继续运行。reference 在 winner 后调用 `stop_source.request_stop()`，帮助支持取消的 loser 尽快退出，再等 remaining 到 0 才恢复 parent。`when_any_test.cpp` 覆盖了 winner 已触发 stop 但 loser 尚未 release 的窗口。
6. `shared_task` 为什么返回拷贝。

   **答案解析：** `shared_task` 让多个 awaiter 读取同一个 producer 结果，结果缓存在 shared state 里。若 `await_resume()` move 返回，第一次消费会改变缓存，后续 awaiter 得到的值就不可靠。reference 返回 `*state->value` 的拷贝，保证所有 awaiter 看到同一成功值。
7. stdexec bridge 为什么需要 atomic phase 处理同步完成。

   **答案解析：** stdexec sender 的 `start()` 可以同步调用 receiver completion，也可能异步完成，还可能在等待协程销毁后才完成。atomic phase 把 `starting/suspended/completed/abandoned` 四种状态分开：同步完成时 `await_suspend` 返回 false，异步完成时 completion 恢复 caller，abandoned 时避免恢复已销毁 caller。reference 的 `stdexec_awaitable.hpp` 正是为这个窗口写的。

本项目完成后，再读 cppcoro、folly coro 或 stdexec task 时，可以把它们的复杂代码映射回这些小组件。
