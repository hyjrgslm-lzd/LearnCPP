# 练习 D14：sender 图与协程桥接对照

D14 不是让你再写一个协程运行时。它把同一段逻辑分别写成 sender 图和 coroutine task，让你观察：协程让局部顺序更像普通代码，但 scheduler、stopped、value/error/stopped 三通道仍然来自 sender-receiver 协议。

## 你要实现什么

命名空间是 `c10_d14`。当前 Reference 提供这些入口，Student 需要保持相同接口：

- `sender_graph(int input)`：返回 sender，把输入乘 2 后转成字符串。
- `coroutine_task(int input)`：返回 `stdexec::task<std::string>`，在协程体内 `co_await just(input)`，产出同样字符串。
- `stopped_source()`：构造一个真实走 stopped channel 的 sender。
- `sender_stopped_status()`：用 `let_stopped` 把 stopped 映射成字符串状态。
- `coroutine_stopped_status()`：在 coroutine 内 `co_await` 同样的 stopped-mapped sender。
- `sender_switch_thread(Scheduler)`：用 `starts_on(scheduler, ...)` 真正切到传入 scheduler。
- `coroutine_switch_thread(Scheduler)`：在 coroutine 内显式 `co_await sender_switch_thread(scheduler)`。
- `stdexec_task()`：返回一个最小 `stdexec::task<int>`，证明固定库 task 可被 `sync_wait` 消费。

## Parts

1. 先写 sender 图。`just(input) | then(*2) | then(to_string)` 保留数据流图结构。
2. 再写 coroutine 版本。`co_await just(input)` 后用普通表达式计算结果；不要把输入写死，checker 会用 fresh input 拒绝常量实现。
3. 写 stopped 映射。stopped 不是 exception，也不是空字符串；sender 图和 coroutine 都必须显式把 stopped channel 转成可观察状态。
4. 写 scheduler 切换。协程不会自动选择线程；必须在协程体内 `co_await starts_on(...)` 形成调度边界。
5. 保留固定库 task 对照。D14 使用 `stdexec::task` 展示真实库接口；H3 的教学 task 是机制实验，不替代这里的标准库/固定库对照。

## checker 覆盖

- `sender_graph(21)` 得到 `"42"`，`sender_graph(5)` 得到 `"10"`。
- `coroutine_task(21)` 与 sender 图结果一致。
- `stdexec_task()` 可被 `stdexec::sync_wait` 消费。
- sender/coroutine 两种 stopped 映射都得到 `"stopped"`。
- `sender_switch_thread` 和 `coroutine_switch_thread` 都运行到 `exec::static_thread_pool` 的 worker 线程，返回的 `std::thread::id` 不等于 caller，并且两者观察到同一个 scheduler 执行来源。
- checker 用 `with_awaitable_senders` / `as_awaitable` 的 probe 静态确认 sender 图能进入协程 await 协议。

## Reference / good / bad

- `src/reference/solution.hpp` 使用固定 stdexec/exec API 写出真实 sender/coroutine 对照。
- `validation/good/solution.hpp` 是独立 good，保持同样接口和语义。复核结果留在本机验证记录中。
- `validation/bad/solution.hpp` 可编译，但会用常量或缺失真实 channel/scheduler 行为，被 `sender graph consumes fresh input` 等检查拒绝。

## 直接命令

```powershell
cmake -S C10_Execution/exercises -B build/c10-d14 -DC10_UNITS="D14_coroutine_bridge" -DFETCHCONTENT_SOURCE_DIR_STDEXEC=$env:STDEXEC_ROOT -DC10_STUDY_BUILD_REFERENCE=ON
cmake --build build/c10-d14 --config Debug --target D14_coroutine_bridge_reference D14_coroutine_bridge_validation_good D14_coroutine_bridge_validation_bad D14_coroutine_bridge_student
ctest --test-dir build/c10-d14 -C Debug --output-on-failure
```

Student 初态应输出 `UNFINISHED: D14 coroutine bridge: sender graph` 并返回 exit 2。
