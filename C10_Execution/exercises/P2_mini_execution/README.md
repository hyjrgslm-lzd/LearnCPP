# 练习 P2：mini execution 子集

P2 是全章的协议压缩版。你不直接调用 stdexec，而是在 `c10_p2::mini` 里写一套小型 sender-receiver：CPO、completion signatures、`just`、`then`、`sync_wait`、`when_all`、run loop scheduler，以及一个把 mini sender 适配到 stdexec 的边界。目标是理解 P2300 的对象关系，不是追求完整库特性。

## 你要实现什么

公开命名空间是 `c10_p2::mini`。Reference 支持以下核心能力：

- CPO：`connect`、`start`、`set_value`、`set_error`、`set_stopped`、`get_env`、`schedule`、`get_allocator`。
- 静态协议：`completion_signatures`、`completion_signatures_of_t`、`value_types_of_t`、`error_types_of_t`，以及 `sender`、`receiver`、`operation_state` concepts。
- 基础 sender：`just(...)`、`just_error(exception_ptr)`、`just_stopped()`。
- adaptor：`then(sender, fn)` 和 pipe closure `sender | then(fn)`。
- 消费端：`sync_wait(sender)`，value 返回 `optional<tuple<...>>`；error rethrow；stopped 返回 `nullopt`。
- 组合：`when_all(left, right)`，按 sender 顺序合并 value；error/stopped 折叠到对应 channel。
- scheduler：`run_loop`、`get_scheduler()`、`schedule(scheduler)`、`close()`。
- interop：`as_stdexec(mini_sender)`，让 mini sender 能被 `stdexec::sync_wait` 消费。

## Parts

1. 写 CPO。优先走成员函数；没有自定义 env 时 `get_env` 返回空 env。CPO 本身应是空对象。
2. 写 completion meta。`just(1)` 的 value types 应是 `variant<tuple<int>>`；`then` 返回 `void` 时 value types 是 `variant<tuple<>>`。
3. 写基础 sender。`just()` 发送 `set_value()`；`just_error` 走 error channel；`just_stopped` 走 stopped channel。
4. 写 `then`。它的 receiver 包住下游 receiver：收到 value 后调用函数，函数返回非 void 则 `set_value(result)`，返回 void 则 `set_value()`；函数抛异常转 `set_error(current_exception())`。
5. 写 env 转发。`then` 的内部 receiver 必须把 `get_env()` 转发给下游 receiver。checker 用 `get_allocator` 自定义 query 验证这一点。
6. 写 `sync_wait`。它连接并启动 sender，用条件变量等待 terminal；value 存 tuple，stopped 存空 optional，error rethrow。
7. 写 `when_all`。左右 sender 都完成 value 后合并 tuple；任一 error 走 error；任一 stopped 走 stopped。结果顺序按参数顺序，不按完成先后。
8. 写 run loop scheduler。`schedule(sch)` 发送 void value；completion 运行在 `run_loop.run()` 线程；`close()` 后 schedule 报 `runtime_error("closed")`，且能送到 move-only receiver。
9. 写 stdexec 适配。`as_stdexec(mini::just(17))` 能被 `stdexec::sync_wait` 得到 17。

## checker 覆盖

- CPO 空对象与基本 concepts。
- `completion_signatures_of_t`、`value_types_of_t`、`error_types_of_t` 的静态形状。
- `then` 链式计算：`20 -> 42`，fresh input `3 -> 8`，每级回调只执行一次。
- `just()` + `then` 返回 value；void callback 映射成 `set_value()`。
- `just_error` 被 `sync_wait` rethrow；`just_stopped` 返回 `nullopt`。
- 直接 `connect/start` 驱动 receiver。
- env query 通过 `then` receiver 转发。
- pipe syntax 顺序组合。
- mini sender 适配到 stdexec。
- run loop scheduler 在线程上完成；`when_all` 保持参数顺序、折叠 error/stopped；closed run loop 对 move-only receiver 发送 error。

## Reference / good / bad

- `src/reference/solution.hpp` 是完整教学实现。
- `validation/good/solution.hpp` 是独立 good，实现相同 mini 协议。
- `validation/bad/solution.hpp` 可编译，但会在 fresh input、channel 或 scheduler 行为上失败；负例诊断包括 `mini pipeline consumes fresh input`。

## 直接命令

```powershell
cmake -S C10_Execution/exercises -B build/c10-p2 -DC10_UNITS="P2_mini_execution" -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT -DC10_STUDY_BUILD_REFERENCE=ON
cmake --build build/c10-p2 --config Debug --target P2_mini_execution_reference P2_mini_execution_validation_good P2_mini_execution_validation_bad P2_mini_execution_student
ctest --test-dir build/c10-p2 -C Debug --output-on-failure
```

Student 初态应编译，并通过 `c10::unfinished` 返回 exit 2。
