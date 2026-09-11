# 10 模块 H：协程与 sender/receiver 桥接

模块 H 解决一个工程拼接问题：前面你已经能手写 `promise_type`、awaiter、`lazy_task` 和 `sync_wait`，现在要把这些能力接到 C++ 标准路线上的 sender/receiver 模型。

sender/receiver 的强项是描述异步操作图。它能把“在哪里运行、如何组合、如何完成、如何取消、环境从哪里来”放进一个统一协议。协程的强项是描述局部控制流。它让一次请求、一段业务逻辑、一次 pipeline 的多个等待点仍然按顺序阅读。桥接层的任务就是把 sender 的三条 completion channel 映射成协程里的返回、异常和停止，再把协程函数本身包装成 sender，让它能被 `sync_wait`、`when_all`、`starts_on` 等组合器消费。

本模块使用 pinned NVIDIA/stdexec `nvhpc-26.05`。它是 P2300/P3552 路线的实现之一，不等同于已经发布的标准库类型。2026-09-11 核对的 WG21 编辑报告里，N5050 是 C++26 DIS 基础，N5054 是 C++29 工作草案；`execution::task`、`execution::as_awaitable` 属于当前工作草案的 execution 章节学习入口。代码里必须按真实命名空间写 `stdexec::` 或 `exec::`，不能把它改名成未来的 `std::execution`。CMake 中的 `H2_std_task_probe` 会在 C++26 preview flag 下真实编译 `std::execution::task<int>` 协程体并尝试 `std::this_thread::sync_wait` 消费，用来观察当前工具链是否已经提供标准 task；H2 的 reference 使用 pinned stdexec task 观察同类语义。

有用的上游入口：

- stdexec reference：<https://nvidia.github.io/stdexec/reference/index.html>
- stdexec user guide：<https://nvidia.github.io/stdexec/user/index.html>
- P3552R3 `task` proposal：<https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3552r3.html>
- P2300 sender/receiver implementation overview：<https://github.com/bemanproject/execution>

<a id="h1"></a>

## H-1：把 sender 适配成 awaitable

H-1 的问题模型很小：协程体里写 `co_await stdexec::just(42)`，但 `stdexec::just(42)` 是 sender，不能直接当普通 awaiter 用。它不会直接提供 `await_ready / await_suspend / await_resume`。你要写一个 `sender_awaitable`，在 `await_suspend` 里把 sender 接到 receiver，再启动 operation state。

真实链路如下：

```text
co_await sender
  -> promise.await_transform(sender)
  -> sender_awaitable{sender}
  -> await_suspend(coroutine_handle)
  -> stdexec::connect(sender, bridge_receiver)
  -> stdexec::start(operation_state)
  -> set_value / set_error / set_stopped
  -> await_resume()
```

这条链里每个对象都有职责。

`sender` 表示“有一件异步事可以被连接”。它本身通常是 lazy 的，创建 sender 不等于开始运行。`receiver` 表示“这件事完成时通知谁”。receiver 必须提供 completion channel：`set_value` 表示成功，`set_error` 表示失败，`set_stopped` 表示被取消或停止。`operation_state` 是 `connect(sender, receiver)` 的结果，它持有运行这张小图需要的状态，必须活到完成。`start(operation_state)` 才真正启动。

H-1 reference 的 `sender_awaitable` 持有 sender、结果 storage、operation state 和一个原子握手状态。receiver 收到 completion 后只做三件事：写结果、标记完成、按握手结果决定是否恢复协程。`await_resume` 只读 storage：value 返回，error rethrow，stopped 抛出教学用 `stopped_error`。

最小结构可以先按这个形状读：

```cpp
template <class Sender, class Promise>
struct sender_awaitable {
    Sender sender;
    std::optional<connect_result_t<Sender, receiver>> op;
    bridge_state state;

    bool await_suspend(std::coroutine_handle<Promise> h) {
        // operation state 必须成为 awaitable 成员，不能放在局部变量里。
        op.emplace(stdexec::connect(std::move(sender), receiver{h, &state}));
        stdexec::start(*op);

        // 如果 sender 在 start() 内同步完成，这里返回 false，让协程继续执行。
        return state.publish_continuation_or_detect_completed(h);
    }

    int await_resume() {
        // completion channel 在这里回到协程语义：value/error/stopped。
        return state.take_value_or_throw();
    }
};
```

这段伪代码要和 `solution.cpp` 对照看。真实 reference 用 `variant` 存储结果，用 atomic 保存握手状态；教学重点是“completion 写状态，恢复路径只选择一次”。`await_suspend(false)` 表示协程已经经过挂起点，随后立即继续到 `await_resume`；`await_ready(true)` 则发生在进入挂起点之前。

关键坑是同步完成。`stdexec::just(42)` 可以在 `start()` 内立刻调用 `set_value`。按语言规则，协程在进入 `await_suspend` 前已经挂起；`await_suspend` 发布 handle 后，同步或并发恢复都有可能发生。难点在于 publication 之后，awaiter 和协程帧的生命周期会被恢复路径影响，递归恢复也会让调用栈变深。reference 选择用原子握手组织这件事：completion 先只写结果和完成状态，`await_suspend` 再决定返回 `false` 立即继续，或返回 `true` 交给 completion 路径恢复。

```text
completion 先到：
  receiver: result = value, state = completed
  await_suspend: 发现 completed，返回 false
  当前协程经过挂起点后立即继续到 await_resume

挂起先完成：
  await_suspend: 保存 coroutine handle，返回 true
  receiver: result = value，看到 handle，resume
  当前协程稍后从 await_resume 继续
```

做 H-1 时先预测三条结果。

- `co_await stdexec::just(42)`：`set_value(42)` 写入 variant，`await_resume` 返回 42。
  **答案解析：** `just(42)` 的 completion channel 是 `set_value`，bridge receiver 只负责把值写进 awaitable 的结果 storage 并完成握手。协程恢复后真正把值交给表达式的是 `await_resume()`，所以业务代码看到的是普通 `int`，不是 sender 或 receiver 对象。
- `co_await stdexec::just_error(exception_ptr)`：`set_error` 写入异常，`await_resume` 重新抛出。
  **答案解析：** error channel 在桥接层不能被吞掉，否则父协程会继续走 value 分支。reference 把 `exception_ptr` 存进结果 variant，`await_resume()` 在 `co_await` 表达式位置重新抛出；这个异常再由当前 task 的 `try/catch` 或 `unhandled_exception()` 处理。
- `co_await stdexec::just_stopped()`：`set_stopped` 写入 stopped tag，`await_resume` 走停止路径。
  **答案解析：** stopped 是独立 completion channel，表达“上游决定不产出 value”。H1 用教学 `stopped_error` 把它带回协程体，方便和异常路径对照；后续 H3/组合器需要保留这条通道，才能把取消和失败分开传播。

如果你把 `operation_state` 放在 `await_suspend` 局部变量里，异步 completion 到达时它已经析构；这是对象所有权错误。如果你漏掉 stopped channel，后续 H-3 和 `when_all` 的取消语义也会断。

<a id="h2"></a>

## H-2：观察 `std::execution::task` / stdexec task

H-2 的目标是把 task 放进 sender/receiver 世界里，重点从自写 `lazy_task` 转到标准路线的组合协议。

P3552R3 提出的 task 是协程返回类型，也是 sender。它的对象可以从协程函数返回；被连接或等待后才启动；完成时通过 sender completion channel 通知下游；协程内部还能从 environment 里拿 scheduler、stop token、allocator 等上下文。这个设计让 task 可以自然进入 `when_all`、`then`、`sync_wait`、`starts_on`。

reference 里的一个典型片段：

```cpp
template <ex::scheduler Scheduler>
stdexec::task<int> fan_in_on_scheduler(Scheduler scheduler) {
    auto [left, right] = co_await ex::when_all(
        ex::starts_on(scheduler, ex::just(20) | ex::then([](int v) { return v + 1; })),
        ex::starts_on(scheduler, ex::just(30) | ex::then([](int v) { return v + 2; })));
    co_return left + right;
}
```

这里 `co_await when_all(...)` 等待的是一个 sender 组合。组合本身 lazy；外层 task 被 `sync_wait` 启动后，两个分支才启动。`starts_on` 安排输入 sender 从指定 scheduler 的 execution resource 上启动；reference 的输入分支是 `just | then`，没有再切到其他资源，所以能观察到 worker 线程执行。`when_all` 负责汇合 value。

H-2 reference 做五个观察。

```text
stdexec::task<int>
  co_await stdexec::just(...)
  -> sync_wait(task)
  -> 拿到 value

exec::static_thread_pool
  starts_on(scheduler, just | then)
  -> 本题分支在 worker 资源上运行

when_all(a, b)
  -> 两个 child sender 都启动
  -> 所有 value 到齐后拼接结果

just_stopped()
  -> stopped_as_optional(...)
  -> std::optional 为空

get_stop_token()
  -> 从 task environment 读取 stop token
```

这里要特别看 `starts_on` 和 `when_all`。stdexec 文档说明 `starts_on(scheduler, sender)` 安排输入 sender 从指定 scheduler 的 execution resource 上启动；如果输入 sender 内部又切到别的 scheduler，它的完成位置仍由输入 sender 自己的组合决定。H-2 reference 的输入是 `just | then` 这种没有再切换资源的简单 sender，所以可以观察到 worker 线程执行。`when_all` 启动所有输入 sender，等所有输入完成后拼接 value；如果某个输入 error 或 stopped，内部 stop source 会请求其他输入停止，最终只给下游一个 completion。

所以 H-2 的观察因果是：

- worker thread id 和 main thread id 不同，是因为分支经 `starts_on(pool.get_scheduler(), ...)` 被调度到 pool。
  **答案解析：** `starts_on` 安排输入 sender 在给定 scheduler 的 execution resource 上启动。H2 的输入分支是 `just | then`，内部没有再切换 scheduler，所以 `then` 里记录到的是 pool worker 线程；如果输入 sender 自己再切资源，completion 位置要按那条组合链继续追。
- `when_all` 的结果是 53，是因为两个分支分别算出 21 和 32，再在 task 中相加。
  **答案解析：** 两个 child sender 都被 `when_all` 连接并启动，value 到齐后组合成 tuple。外层 `task` 在 `co_await when_all(...)` 后拿到 `left=21`、`right=32`，再 `co_return left + right`，所以 `sync_wait` 读到 53。
- `stopped_as_optional` 返回空 optional，是因为 stopped channel 被显式转换成 value channel。
  **答案解析：** `just_stopped()` 本来不会给 value，直接等待会走 stopped completion。`stopped_as_optional` 把 stopped 映射为 `std::optional` 的空值，让调用侧用普通 value channel 表达“没有结果”，适合把取消当可预期分支处理。
- `get_stop_token().stop_possible()` 为 true，说明 task promise 能从环境里读到停止通道。
  **答案解析：** stdexec task 的协程体可以通过 environment 查询上下文，stop token 就来自这条环境边。这里的结论只说明当前 task 运行环境提供了可停止状态，不代表已经请求停止，也不代表所有 sender 都自动响应；响应仍取决于下游是否检查 token 或实现 stopped channel。

如果你的工具链不支持 pinned stdexec task reference，CMake 不创建 reference target。这表示工具链支持边界。starter 和 probe 仍能编译，用来确认当前环境。

<a id="h3"></a>

## H-3：让自写 task 同时是 sender 和 awaitable

H-3 是模块 H 的闭环。你要写 `my_task<T>`，它既能被 sender 消费者这样使用：

```cpp
auto op = stdexec::connect(sender_side(), receiver);
stdexec::start(op);
```

也能被另一个协程这样使用：

```cpp
my_task<int> outer() {
    int v = co_await inner();
    co_return v * 2;
}
```

还要能在自己的协程体里等待外部 sender：

```cpp
my_task<int> bridge_side() {
    int v = co_await stdexec::just(42);
    co_return v + 1;
}
```

这三条路径的出口不同。

```text
sender consumer path:
  connect(my_task, receiver)
    -> op_state owns task + receiver
    -> start(op_state)
    -> resume coroutine
    -> final_suspend calls set_value/set_error/set_stopped(receiver)

awaitable path:
  outer co_await inner
    -> await_transform(my_task&&)
    -> task_awaiter saves continuation
    -> symmetric transfer to child
    -> child final_suspend returns continuation
    -> outer await_resume reads result

sender-inside-task path:
  my_task body co_await stdexec::just(42)
    -> promise.await_transform(sender)
    -> reuse H-1 sender_awaitable
```

reference 把 `external_receiver` 和 `continuation` 分开。`external_receiver` 只服务 sender 路径：`final_suspend` 看见它，就向 receiver 发 `set_value / set_error / set_stopped`。`continuation` 只服务 awaitable 路径：子协程结束时把控制权交回等待它的父协程。

`final_suspend` 的最小心智模型：

```cpp
auto final_suspend() noexcept {
    struct awaiter {
        std::coroutine_handle<> await_suspend(handle h) noexcept {
            auto& p = h.promise();
            if (p.external_receiver) {
                // sender 消费路径：把 promise 里的结果翻译成 completion channel。
                p.complete_receiver();
            }
            // awaitable 消费路径：把控制权交回 co_await 它的父协程。
            return p.continuation;
        }
    };
    return awaiter{};
}
```

如果 `external_receiver` 和 `continuation` 混在一个出口里，sender 消费和协程消费会互相污染：一种表现是 `sync_wait(my_task)` 永远收不到 `set_value`，另一种表现是子协程结束后父协程不恢复。

另一个关键点是避免把 `my_task` 直接做成通用 awaiter。pinned stdexec 会检查类型是否像 awaitable；如果 `my_task` 同时裸露 `await_ready / await_suspend / await_resume`，stdexec 可能走普通 awaitable 适配，绕开你写的 `connect`。reference 的做法更窄：`my_task` 作为 sender 公开 `connect`；作为 awaitable 只在自己的 `promise_type::await_transform(my_task&&)` 中返回 `task_awaiter`。

H-3 的三个预期值：

- `sender_side()` 经 `connect/start` 返回 42。
- `outer()` 内部 `co_await inner()`，`inner` 返回 10，`outer` 返回 20。
- `bridge_side()` 内部 `co_await stdexec::just(42)`，桥接后返回 43。

如果某条路径卡住，按出口查：sender 路径查 `external_receiver` 是否设置和 completion signatures 是否匹配；awaitable 路径查 continuation 是否被保存并从 `final_suspend` 返回；内部 sender 路径查 H-1 的同步完成握手。

H1-H3 的 starter 不是“打印 smoke 就算完成”。打开 `COROUTINE_STUDY_TEST_STARTERS=ON` 后，测试会实际消费学生实现，并且 checker 与学生入口分离：学生只改各题 `student.hpp`，`checks/main.cpp` 提供不可改的输入、协程体和 private counters。H1 在 checker 协程体里 `co_await` private fixture 提供的 traced sender，逐个比较多组输入输出，并要求 `connect/start/completion` 计数发生；H2 把真实 sender、scheduler、stopped task、token task 交给学生函数，counter 保留在 checker 闭包/协程体内，要求 value、线程 hop、`when_all`、`stopped_as_optional`、stop token 查询都来自实际执行；H3 用 checker 固定的三条链验证 `my_task` sender 消费、嵌套 `co_await my_task` 和 task 内部 `co_await sender`，task-body 和 bridge sender 计数不作为可写参数暴露。未完成 starter 会有限返回非 0；完成 TODO 后用同一测试自然转绿，不需要完成标记。各题 `validation/good` 与 `validation/bad_constant` 用同一 checker 编译，证明 reference 隔离和常量/删 await 反例不能通过。

## 构建与运行

```powershell
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build-h -G Ninja -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build C09_Coroutines/exercises/build-h --target H1_as_awaitable_reference H2_std_task_probe H2_std_execution_task_reference H3_bidirectional_bridge_reference
ctest --test-dir C09_Coroutines/exercises/build-h -R "H[123].*_reference" --output-on-failure
```

MSVC 生成器示例：

```powershell
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build-h-release -G "Visual Studio 18 2026" -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build C09_Coroutines/exercises/build-h-release --config Release --target H2_std_task_probe H2_std_execution_task_reference
ctest --test-dir C09_Coroutines/exercises/build-h-release -C Release -R H2_std_execution_task_reference --output-on-failure
```

检查 starter：

```powershell
python C09_Coroutines/exercises/tools/configure_windows.py --build C09_Coroutines/exercises/build/h-student --light --student
cmake --build C09_Coroutines/exercises/build/h-student --config Release --target H1_as_awaitable H2_std_execution_task H3_bidirectional_bridge
ctest --test-dir C09_Coroutines/exercises/build/h-student -C Release -R "^(H1_as_awaitable|H2_std_execution_task|H3_bidirectional_bridge)$" --output-on-failure
```

初始 TODO 版本预期失败，失败文本应来自对应行为检查。不要把这组失败当环境坏；它证明 checker 真在调用学生实现。

学完本模块，你应该能画出一条完整链：`co_await sender -> receiver completion -> coroutine resume`，也能反向说明一个协程 task 如何变成 sender 被外部组合器启动。

**答案解析：** 正向链从 promise 的 `await_transform(sender)` 开始，经 `connect(sender, receiver)` 得到 operation state，`start` 后由 receiver 的 `set_value/set_error/set_stopped` 写入结果并恢复协程，最后在 `await_resume()` 映射成返回、抛异常或停止。反向链从 `connect(my_task, receiver)` 开始，operation state 拥有 task 和 receiver，`start` 恢复 task 协程，`final_suspend` 读取 promise 的完成状态并调用外部 receiver 的 completion channel。
