# 15 mini execution 子集

P2 的目标不是复刻 stdexec，也不是写一个只会返回 `42` 的玩具函数。它要把 sender/receiver 的骨架缩到读者能一次读完的大小，同时保留真正的对象关系：sender 描述一次工作，receiver 接收完成信号，`connect` 把两者绑成 operation state，`start` 才启动。只要这条链是真实的，后续再读 stdexec、P2300 或运行时源码时，名字会变多，但核心关系不会变。

本单元的 mini 库限定值域：value channel 支持 `void`、`int` 和两个 sender 合并后的两个输入；error channel 使用 `std::exception_ptr`；stopped channel 用空结果表示。这个限制是教学边界，不是协议偷懒。实现仍然要求 CPO 对象、completion signatures、`value_types_of_t` / `error_types_of_t`、sender/receiver/operation_state concept、non-movable operation state、三通道传播、环境透传、scheduler、`when_all`、pipe 和一处 stdexec 互操作。

## 架构图

```text
调用表达式
  mini::sync_wait(mini::then(mini::just(42), f))

        CPO 层
connect/start/set_value/set_error/set_stopped/get_env/schedule/get_allocator
        |
        v
        类型层
completion_signatures<...>
completion_signatures_of_t<S, Env>
value_types_of_t<S> / error_types_of_t<S>
sender / receiver / operation_state
        |
        v
        sender 层
just / just_error / just_stopped
then / when_all / pipe adaptor
run_loop scheduler schedule sender
        |
        v
        operation state
connect(sender, receiver) 生成不可移动对象
start(op) 恰好启动一次
        |
        v
        receiver 终端
set_value(values...) / set_error(exception_ptr) / set_stopped()
sync_wait 收束为 optional<tuple<...>>
```

CPO 必须是对象而不是函数模板，因为协议的调用点需要一个稳定名字：`mini::connect(s, r)` 是表达式，`decltype(mini::connect)` 也是类型。函数模板只能被调用，不能作为定制点对象被检测、传递或组合。mini 采用 member dispatch：CPO 内部调用 `sender.connect(receiver)`、`op.start()`、`receiver.set_value(...)`。这比完整 tag_invoke 少一层，但保留了定制点对象和协议边界。

## Part 1：CPO 和类型层

先实现 `connect_t`、`start_t`、`set_value_t`、`set_error_t`、`set_stopped_t`、`get_env_t`、`schedule_t` 和一个小的 `get_allocator_t`。每个 CPO 都是 `inline constexpr` 对象。`get_env` 没有成员时返回 `empty_env`，这样简单 receiver 不必写环境；需要环境的 adaptor receiver 再把下游环境透传给上游。

类型层使用函数类型表达完成签名：

```cpp
completion_signatures<
  set_value_t(int),
  set_error_t(std::exception_ptr),
  set_stopped_t()
>
```

`value_types_of_t<S>` 从签名中提取所有 `set_value_t(...)`，本题约定返回 `std::variant<std::tuple<...>>`。`error_types_of_t<S>` 提取 `set_error_t(E)`。`then` 的签名变换在编译期完成：上游 `set_value_t(Args...)` 变成调用 `F(Args...)` 后的 `set_value_t(Result)`；如果 `F` 返回 `void`，输出 `set_value_t()`；上游 error/stopped 原样转发，并额外加入 `set_error_t(std::exception_ptr)` 表示 transform 可能抛异常。

## Part 2：基础 sender

`just(values...)` 存一组值，`connect` 后生成不可移动 operation state，`start` 时把值移动给 receiver：

```text
just(1)
  connect(receiver)
    -> op{value=1, receiver}
  start(op)
    -> set_value(receiver, 1)
```

`just()` 是合法 sender，它发 `set_value()`，也就是 void 成功。`just_error(err)` 只发 `set_error(err)`；`just_stopped()` 只发 `set_stopped()`。这三者让 checker 能区分成功、错误、停止三条通道，而不是把所有失败都塞进异常返回值。

operation state 删除 copy/move 构造和赋值。原因很具体：scheduler 会把 operation state 的地址入队。如果对象可移动，队列里的指针可能指向旧地址。mini 的 run_loop 用稳定 op 指针入队，关闭时在 receiver 尚未 move 前报告 `"closed"` 错误；队列分配失败时也通过 error channel 返回。

## Part 3：then adaptor

`then(sender, f)` 不自己启动上游。它在 `connect` 时把下游 receiver 包成一个内部 receiver，再连接上游：

```text
then(S, F).connect(R)
  inner_receiver = { downstream R, function F }
  inner_op = connect(S, inner_receiver)
  outer_op = { inner_op }

start(outer_op)
  start(inner_op)
```

内部 receiver 收到 `set_value(args...)` 后调用 `F(args...)`。返回非 void 时把结果发给下游；返回 void 时调用下游 `set_value()`。如果 `F` 抛异常，catch 后调用下游 `set_error(std::current_exception())`。上游 error/stopped 不调用 `F`，直接透传。

`then_receiver::get_env()` 返回下游 receiver 的环境。这样上游 sender 可以通过 `get_env` 读到下游提供的 query。checker 用 `get_allocator` 做最小验证：上游 sender 读取环境中的 id，`then` 转换后仍能得到正确值。

pipe 语法只是一个 adaptor 对象：

```cpp
mini::just(10)
  | mini::then([](int v) { return v + 1; })
  | mini::then([](int v) { return v * 2; });
```

它不引入新协议，只把 `sender | then(f)` 转回 `then(sender, f)`。

## Part 4：sync_wait

`sync_wait(sender)` 是消费端。它创建一个 wait state 和 receiver，连接 sender，启动 operation state，然后等待终端信号。成功时返回 `std::optional<std::tuple<...>>`；停止时返回 `std::nullopt`；错误时重新抛出 `std::exception_ptr`。

并发边界在这里很容易写错。终端回调必须在锁内写入结果、设置 `done` 并通知；解锁后不再访问 wait state。等待线程被唤醒后可能立刻销毁 wait state，如果回调在 notify 后继续碰它，就会把“看起来同步”的代码写成 use-after-free。

## Part 5：run_loop 和 scheduler

`run_loop::get_scheduler()` 返回轻量 scheduler。`mini::schedule(sch)` 返回 sender，它的完成签名是 `set_value_t()`，不是 `set_value_t(int)`。这点很重要：调度本身只表示“切到这个执行资源上继续”，不应该发一个假的 token。所以下游写法是：

```cpp
mini::run_loop loop;
auto sch = loop.get_scheduler();
auto work = mini::then(mini::schedule(sch), [] { return 100; });
```

`schedule` 的 operation state 继承一个内部 queue node。`start` 成功时把 `this` 入队；`run()` 在线程中取出 node 并调用 `run_node()`，最终向 receiver 发 `set_value()`。关闭后的 `start` 走 error channel；已经入队的任务仍由 `run()` 收束。

## Part 6：when_all 两路组合

本题只实现两个 sender 的 `when_all(s1, s2)`。它的职责是等两边都完成，然后按输入 sender 顺序合并 value：

```cpp
auto both = mini::when_all(
  mini::then(mini::schedule(sch), [] { return 1; }),
  mini::just(2)
);
// 右边可能先完成，输出仍是 tuple{1, 2}
```

实现上，两个 child receiver 共享一个 state：分别记录左/右 value、第一条 error、是否 stopped、完成计数。任一 child 完成时只更新共享状态；只有第二个 child 完成时才向下游发送终端信号。error 优先于 stopped，stopped 优先于 value。这个 mini 版本不做取消传播，所以 error 也等待另一边结束；这是有意的教学限制，避免把结构化取消和 stop token 的完整协议塞进一个小项目。

终端回调可能销毁外层 operation state，因此发送 `set_value/set_error/set_stopped` 之后不能再访问 `this` 或共享状态。Reference 在发终端信号前把需要的数据移出锁保护区，之后只调用下游 CPO 并立即返回。

## Part 7：stdexec 互操作边界

Reference 的主体不 alias stdexec。它有自己的 CPO、签名类型、sender、operation state、run_loop 和组合算法。为了让读者看到边界，提供一个小的 `as_stdexec(mini::just(...))` 适配入口，把 mini 的 `just` 转成 stdexec sender，再交给 `stdexec::sync_wait` 消费。

这不是说 mini sender 天然就是 stdexec sender。真正工业互操作需要环境、停止令牌、scheduler query、completion scheduler、domain、自定义 tag_invoke 等更完整的适配层。本题只保留一个可运行切口：证明“协议对象可以被边界适配”，同时不把 Reference 主体偷换成 stdexec。

## Checker 覆盖点

P2 checker 现在直接消费 `c10_p2::mini`，并验证：

- CPO 是对象，`completion_signatures`、`value_types_of_t`、`error_types_of_t` 能在编译期查询。
- `just`、`just_error`、`just_stopped` 三通道可到达 `sync_wait`。
- `then` 的签名变换发生在编译期，运行期能处理 `void` 和 `int`。
- operation state 不可移动。
- `get_env` 和 `get_allocator` query 能穿过 `then_receiver`。
- `run_loop.get_scheduler()` 与 `mini::schedule(sch)` 发 `set_value()`，下游 `then([] { ... })` 在线程中运行。
- `when_all` 在右侧先完成时仍保持输出顺序，并正确收束 error/stopped。
- pipe 语法按顺序组合。
- mini `just` 能通过边界适配给 stdexec 的 `sync_wait`。

Student 起点保留完整协议名和类型层，让读者面对真实接口，而不是只改一个完成标记。默认运行会从 `just` 抛出 `c10::unfinished`，返回 2。Good 是独立 oracle；Bad 是安全的错误实现，会编译但因固定结果被行为检查拒绝。
