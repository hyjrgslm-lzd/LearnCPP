# 练习 H-3：coroutine task 与 sender await 桥

H3 是本章最密集的实现题。你要写一个 lazy `c10_h3::task<T>`，它既能在协程内部 `co_await sender`，也能被外部当作 sender `connect/start`。这不是“返回 42 的协程包装器”；checker 会构造同步、异步、跨线程、void、error、stopped、嵌套 task、环境查询和 terminal 销毁 operation state 的场景。

## 教学域边界

本题保持小而明确的域：value channel 支持 0 个 value（`set_value()`）或 1 个 value（如 `int`、`std::thread::id`、`std::exception_ptr` 作为普通 value）。error channel 使用 `std::exception_ptr`，stopped 是独立 channel。Reference 的 env 支持 `stdexec::inplace_stop_token` 与一个 type-erased scheduler；其他 stop token 类型需要显式 callback bridge，本题不伪装成通用 production task。

## 你要实现什么

- `task<T>` / `task<void>` 持有 coroutine handle，`initial_suspend()` lazy，不自动启动。
- `sync_wait()` resume coroutine，并等待 promise 在 `final_suspend` 发布完成。
- `await_transform(sender)` 把 sender 变成 awaiter：原地构造 inner operation state，`start()` 后挂起，bridge receiver 收到 terminal 后恢复 coroutine。
- `operator co_await()` 支持 task 嵌套：child promise 保存 parent continuation，`final_suspend` 对称转移回 parent。
- `task` 自身是 sender：提供 `sender_concept`、`get_completion_signatures`、`connect` 和 operation state 的 `start()`。
- `promise.get_env()` 给 awaited sender 暴露 stop token 与 scheduler。task-as-sender 的 `start()` 必须先从 connected receiver 的 env 复制这些信息，再启动 coroutine。

## Parts

1. 基本 frame 生命周期。`get_return_object()` 生成 task；task move 后源 handle 置空；析构时 destroy 未消费 frame。
2. `final_suspend`。先把 continuation/completion 回调复制到局部，再在锁内设置 `done` 并 notify。发布完成后不能再读 promise 成员，因为 waiting receiver 可能销毁外层 operation state 和 coroutine frame。
3. sender awaiter。`await_suspend` 中 `connect(sender, receiver)` 得到的 operation state 必须原地存放在 awaiter 内，支持 immovable op。receiver 的 `set_value` 存入 variant；如果移动 value 进 variant 抛异常，要转成 error channel 再 resume，不能在 `noexcept` terminal 中 terminate。
4. channel 分流。`set_value()` 映射为 void resume；`set_error(std::exception_ptr)` rethrow；`set_stopped()` 设置 stopped 标记并用内部 `stop_unwind` 退出协程栈，外部 `sync_wait` 看见 `c10_h3::stopped`。
5. task-as-sender。`start()` 不能用 `sync_wait()` 阻塞；它配置 promise env 后直接 resume coroutine。同步完成可以在 `start()` 内 terminal，但 terminal 后 `start()` 不得再访问 operation state。
6. env。Reference 复制 receiver 的 `inplace_stop_token`，所以 start 后请求 stop 仍能被 awaited sender 观察到；receiver env 有 scheduler 时必须能转换到本题的 type-erased scheduler，否则编译期拒绝，不静默退回 inline scheduler。

## checker 覆盖

- `co_await just(int)`、fresh input、immovable operation state。
- `std::exception_ptr` 作为 value channel，不误判为 error。
- nested task value/error/stopped。
- void sender 与 `task<void>`。
- promise env 的 stop token 与 scheduler 查询。
- task-as-sender 的 `connect/start`，包括 receiver 只需 move-construct、不可 move-assign。
- `start()` 遇到 pending awaited sender 必须返回；手动完成后再 terminal。
- pre-stop：receiver token 已请求 stop 时不启动 coroutine。
- live stop：coroutine 已启动且 awaited sender pending 时，请求同一个 `inplace_stop_source`，awaited sender 通过 env 观察到 stopped。
- scheduler propagation：receiver scheduler 被 awaited sender 取到，并通过 `schedule` 跑到 pool 线程。
- terminal 回调销毁外层 operation state 后，Reference 不再访问它。

## Reference / good / bad

- `src/reference/solution.hpp` 是本题完整教学实现。
- `validation/good/solution.hpp` 采用独立的固定库机制：基于 `exec::basic_task` 和自定义 context 桥接 `get_scheduler/get_start_scheduler`，用 boxed value 避免 `std::exception_ptr` value 与 error 表示碰撞，并通过真实 `stdexec::connect(inner, bridgeReceiver)` 驱动 task-as-sender。它不使用 detached thread，也不通过跳过 env/live-stop/scheduler 来降低验收。实际验证记录留在本机验证目录中。
- `validation/bad/solution.hpp` 可编译，但没有真实 await/continuation/task sender 行为；负例仍被 `task awaits synchronous sender value` 等行为检查拒绝。

## 直接命令

```powershell
cmake -S C10_Execution/exercises -B build/c10-h3-completion -DC10_UNITS="H3_coroutine_task;D14_coroutine_bridge" -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT -DC10_STUDY_BUILD_REFERENCE=ON
cmake --build build/c10-h3-completion --config Debug --target H3_coroutine_task_reference H3_coroutine_task_validation_good H3_coroutine_task_validation_bad H3_coroutine_task_student
ctest --test-dir build/c10-h3-completion -C Debug --output-on-failure
```

ASan 小范围：

```powershell
cmake -S C10_Execution/exercises -B build/c10-h3-completion-asan -DC10_UNITS="H3_coroutine_task;D14_coroutine_bridge" -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT -DC10_STUDY_BUILD_REFERENCE=ON -DC10_STUDY_ENABLE_ASAN=ON
cmake --build build/c10-h3-completion-asan --config Debug --target H3_coroutine_task_reference H3_coroutine_task_validation_good H3_coroutine_task_validation_bad
ctest --test-dir build/c10-h3-completion-asan -C Debug --output-on-failure
```

Student 初态应输出 `UNFINISHED: H3 task: implement sync_wait` 并返回 exit 2。

教学支持域：awaited sender有零或一个拥有值、error载荷为std::exception_ptr；task支持T和void。环境传播支持inplace_stop_token及unstoppable token，其他token需显式callback桥接；scheduler须能转换到本题使用的type-erased scheduler。并不宣称实现标准task的全部属性与完成签名组合。
