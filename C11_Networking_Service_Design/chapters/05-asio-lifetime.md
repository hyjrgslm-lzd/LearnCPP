# 05 Asio：把完成接回控制流

先修：第 4 章，以及 C09 的 await 协议与帧存活。sender 语法推迟到第 12 章。对应 [L06](../exercises/L06_asio/README.md)。本课固定 Boost1.92，C09 的 standalone Asio 在独立目标中使用。

## 1. handler 与 executor

异步发起函数登记操作并返回；完成时 executor 决定 handler 在何处执行。io_context 是执行上下文，调用 run 的线程负责处理就绪工作。若没有 work 或 pending operation，run 可以返回，因此后台 owner 线程需先建立工作或 work guard。线程数量不是连接数量；连接所有权也不会因 executor 存在而自动变成线程安全。

一个 handler 可再次发起操作，形成读 → 解析 → 写 → 下一读的链。缓冲必须存活到 handler；把局部数组传给 async_read_some 然后返回，是典型悬空借用。将缓冲放进 session，handler 捕获 shared_ptr 可保活；父对象若又强持有所有子对象，需检查是否形成环。

`post` 把工作排队，`dispatch` 允许在符合 executor 条件时内联调用。用 dispatch 不能假设当前栈已经退出，再设 `writing=true`；应先建立状态再暴露给可能完成的路径。排队本身也可能分配失败，所以“标 running 后 post 到 worker”不是无条件安全的事务。

## 2. strand 保护的是执行序列

多个线程 run 同一 io_context 时，无同步地访问连接写队列会产生数据竞争。strand 保证绑定其上的 handler 不并行执行，但不保证它们永远在同一线程，也不保护绕过 strand 的直接调用。异步操作的所有发起、完成、取消和关闭应沿同一串行域。

L06 的主实验同时观察协程 echo、strand 串行更新和 cancellation slot。P1 更简单：只用一个 owner run 网络和 registry，两个工作线程只接收拥有的 work 并回传完成。此时再给每次 registry 操作上锁不会增加正确性；真正需要同步的是 worker 邮箱和进度字段。

## 3. awaitable 只是换一种表达

```cpp
boost::system::error_code ec;
auto n = co_await socket.async_read_some(
    boost::asio::buffer(bytes),
    boost::asio::redirect_error(boost::asio::use_awaitable, ec));
```

挂起期间 bytes 和 socket 仍被操作借用。若 bytes 是协程局部变量，它保存在帧中；销毁帧前仍必须取消并等待 pending read 完成。redirect_error 把操作错误写到 ec；不使用它时 use_awaitable 通常经异常传播系统错误。两种表达不改变底层失败条件，也不自动吞掉取消。

co_spawn 的完成处理负责收集 exception_ptr。detached 可以是明确丢弃结果的选择，但不能用来绕过必须等待的服务生命周期。返回一个 awaitable 后，创建它的短命协程 lambda 捕获对象未必存活；本课驱动将捕获者保留到 run 结束，库接口优先使用拥有参数和 session 成员。

L06 的 runtime/startup 分支使用任务域。首次阅读可先完成本章 Part A；下述任务交接实现及其关闭实验，读完第 10、11 章的接纳、live 和取消契约后回读，不作为理解基础 Asio 的先修。

## 4. deadline 是操作组合的一部分

一个 read 和一个 timer 竞争时，赢家先决定原因，再取消另一方，最后等待两条完成路径都归还借用。若 timer 赢后立刻返回并销毁缓冲，迟到 read completion 仍可能访问旧内存。反过来，read 已完成、timer 取消回调稍后到来，不应把成功覆盖成超时。

必须区分连接 idle timeout、单次请求 deadline、任务执行预算、服务 drain deadline。请求断线不代表远端未接受任务；取消请求等待也不等于取消已接纳任务。所有进程内预算用 steady_clock 的绝对时间，重试/分片循环共享它，避免每次重置 3 秒导致无限等待。

L06_runtime 提供受控 gate，让任务确实处于运行状态，再检查取消和关闭；L06_startup 检查启动失败后的重试与 owner 延迟。gate 只存在于测试构造器，不属于远端可设置的任务字段。关闭在 owner 恢复执行后才被观察到时，也必须比较原始截止时间，不能因为队列刚好已空就忘记已经超时。

## 5. 任务运行时的最小可靠交接

`task_runtime.hpp` 用两条 jthread 对应两个固定 slot。owner 找到空 slot 后将 registry 状态改为 running，移入 work，唤醒 worker；移入既有 optional 不需要再分配。worker 完成后把结果写回同一个 slot，owner 定时收取。这样消除了“post 分配失败，状态已 running 但工作从未提交”的裂缝。

这个实现每 5 ms 扫描两个 slot，选择清晰的有界生命周期，未声称极低延迟。mutex 保护 job/result/busy 交接；task control 的 atomic progress 只是单调计数，不发布其他对象，所以 relaxed 足够。stop_source 表达请求，终态由 owner 在收到完成后确定。工作线程不能直接修改 registry vector，否则 find 返回的指针、迭代器和计数都失去串行前提。

## 6. 自测与解析

1. **strand 能否保护另一个线程直接调用 session.close？** 不能；应将关闭交给该串行域，且处理提交可能失败的责任。
2. **co_await 返回是否总意味着 socket 已关闭？** 只说明该 await 对应操作完成。连接可继续使用，也可能需要处理错误后关闭。
3. **为何不把 worker 完成再 post 回 owner？** 可以，但必须处理 post 的分配失败；本课固定邮箱用少量轮询成本换取无分配结果交接。
4. **完成数据与 stop 同时到达如何选择？** 契约先规定优先级。网络 receive 桥接保留成功读取；任务 registry 保留 owner 已记录的取消/过期原因。不能用一个全局“stop 永远优先”覆盖所有层。
