# 10 task、scope 与协程桥接

协程给 C++ 一个顺序化的写法，但它没有取代 sender/receiver。`co_await sender` 仍然要完成同一件事：把 sender 和 receiver 连接成 operation state，启动 operation state，然后等 value、error 或 stopped 三种 completion 中的一种回来。协程语法隐藏了“挂起当前函数、稍后恢复”的样板；sender/receiver 仍然定义 work 如何启动、在哪个 scheduler 上执行、如何停止、如何把结果送到下游。

理解 task 要从 coroutine frame 开始。调用一个 coroutine 函数时，函数体通常不会立刻执行；编译器先分配 coroutine frame，在里面放 promise、参数副本、局部变量和挂起点状态。`promise_type::get_return_object()` 把 frame 包成一个用户可见的对象，例如 `task<T>`。`initial_suspend()` 决定 coroutine 创建后是否立即运行；H3 的教学 task 用 `suspend_always`，所以它是 lazy task，只有 `sync_wait`、`co_await task` 或 task-as-sender 的 `start()` 才会 resume 它。`co_return value` 调到 `return_value`，未捕获异常调到 `unhandled_exception`，最后进入 `final_suspend`。只要 `final_suspend` 返回后 frame 仍可能被外部观察，task 析构时必须负责 destroy handle；如果 `final_suspend` 用 `suspend_never` 自动销毁 frame，等待者手里就可能拿到悬空 handle。

`final_suspend` 是生命周期最容易出错的地方。H3 的 Reference 在 promise 中保存两类等待者：一种是 `sync_wait` 的条件变量等待者，另一种是嵌套 task 的 parent continuation 或 task-as-sender 的 downstream receiver completion 回调。正确顺序是先把 continuation/completion 回调复制到局部变量，再在锁内设置 `done` 并 notify，最后只使用局部副本恢复 parent 或完成 receiver。这样做的原因很具体：一旦 `done` 被发布，另一个线程可能立刻销毁等待中的 task；一旦 receiver terminal 被调用，下游也允许销毁外层 operation state。发布完成之后再读 promise 或 operation state，就是 use-after-free 窗口。

`co_await sender` 的协议可以拆成四个状态。第一，`await_transform(sender)` 在 promise 上被调用，它决定这个表达式如何变成 awaiter。第二，`await_suspend(coroutine_handle)` 连接 sender 和 bridge receiver，得到 operation state，并启动它。第三，bridge receiver 的 `set_value`、`set_error` 或 `set_stopped` 保存 completion channel，然后 resume coroutine。第四，coroutine 回到 `await_resume()`，它读取保存的 channel：value 返回给用户代码，error rethrow，stopped 走停止路径。H3 限定 value 域为 0 或 1 个 value，所以 `set_value()` 对应 `void await_resume()`，`set_value(T)` 对应返回 T；多 value 和多 alternative 不在本题域内。

operation state 的存活期不能靠临时变量。很多 sender 可以异步完成，`await_suspend` 返回时 operation state 仍被底层系统持有；它必须原地存放在 awaiter 或其他与 coroutine frame 同寿命的位置。H3 的 Reference 使用对齐存储保存 inner operation state，支持不可移动 operation state。同步完成也不能被忽略：`start()` 可能在 `await_suspend` 返回前调用 receiver 并 resume coroutine，所以 awaiter 的代码不能假设“挂起返回以后才会完成”。

completion channel 要按 channel 区分，而不是按类型猜。`std::exception_ptr` 可以是 error channel 的错误对象，也可以是 value channel 的普通值。H3 checker 专门 `co_await just(make_exception_ptr(...))`，再从返回值里 rethrow payload，验证实现没有因为类型名叫 `exception_ptr` 就把 value 当 error。实现上，bridge receiver 用 variant 的索引表示 monostate/value/error/stopped；`set_value(Result)` 如果移动 value 进 variant 时抛异常，要把这个失败转成 error channel，再 resume coroutine，不能在 `noexcept` terminal 中 terminate。

嵌套 task 需要 continuation。`co_await child_task` 时，parent coroutine 挂起，child promise 记录 parent handle，然后返回 child handle 给 coroutine machinery。child 跑到 `final_suspend` 后返回 parent continuation，实现对称转移。这个路径不是“开一个新线程”；它只是把当前 resume 权交给 child，再在 child final suspend 时交回 parent。H3 的 task-to-task awaiter 在转移前也会复制 parent env，让 child 内部 awaited sender 能看见 parent 的 stop token 和 scheduler。

environment 是 sender 与执行上下文之间的查询通道。bridge receiver 的 `get_env()` 不应返回空环境；它应该让 awaited sender 看到当前 promise 的 stop token 和 scheduler。H3 Reference 的边界是明确的：stop token 支持 `stdexec::inplace_stop_token` 与 unstoppable token；如果用户传入其他 stop token 类型，需要额外 callback bridge，本题编译期拒绝而不是假装支持。scheduler 支持可转换成本题 type-erased scheduler 的 scheduler；receiver env 明明有 scheduler 但不能转换时，也编译期拒绝，不静默退回 inline。checker 同时覆盖 pre-stop 和 live stop：pre-stop 时 task-as-sender 不启动 coroutine；live stop 时 coroutine 已启动、awaited sender pending，随后请求同一个 `inplace_stop_source`，awaited sender 通过 env 观察到 stopped。

task 作为 sender 是另一条入口。`task<T>` 的 `connect(receiver)` 返回 operation state，`start()` 必须启动 coroutine，并最终向 receiver 发送 value/error/stopped。H3 Reference 的 `start()` 不调用 `sync_wait`，因为那会阻塞调用线程，也会在同线程 scheduler 图里死锁。它从 receiver env 复制 stop token 和 scheduler，设置 completion 回调，然后 resume coroutine。如果 coroutine 同步完成，terminal 可以发生在 `start()` 内；terminal 之后 `start()` 不能再访问 operation state，因为 receiver 合法地可以在 terminal 回调中销毁它。checker 用一个会销毁外层 op 的 receiver 验证这一点。

D14 展示的是表达层对照。`sender_graph(int)` 用 `just | then | then` 保留数据流图；`coroutine_task(int)` 用 `co_await just(input)` 写成顺序代码。两者结果相同，但调度和 channel 没消失。stopped 仍要用 `let_stopped` 显式映射成状态字符串；scheduler 切换仍要写 `starts_on(scheduler, ...)`，coroutine 版本只是 `co_await` 这段 sender。checker 用 `exec::static_thread_pool` 比较线程 id，确认 sender 和 coroutine 都真的跑到 pool 线程。

scope 操作用来管理“已经启动但还没结束”的工作。一个 sender 图如果只是被构造出来，它还没有运行；一旦 `spawn` 或手动 `start`，operation state 的生命周期就要被某个所有者追踪。R1 直接运行固定 stdexec 的 `simple_counting_scope`、`counting_scope`、`associate`、`close`、`join`、`spawn`、`spawn_future` 和 `request_stop`：`associate` 把 work 计入 scope，`close` 拒绝 late work，`join` 等已有 work drain，`request_stop` 把停止请求传播给关联 work。scope 的限制也要讲清楚：它不替你决定线程，不替你吞掉错误，也不让 dangling operation state 变安全；它只是给已关联 work 一个共同的生命周期边界。

P2 的 mini execution 把这些概念压缩到一个小库里。CPO 决定协议入口，completion signatures 决定静态通道形状，`then` 展示 receiver 包装与 env 转发，`sync_wait` 展示阻塞消费，`when_all` 展示多 sender value/error/stopped 折叠，run loop scheduler 展示 operation state 地址稳定与线程归属。H1 则把 scheduler 缩到最小：`schedule()` sender 的 operation state 入 intrusive FIFO，`run()` 线程 drain，`close()` 后已入队 work 仍执行，late schedule 走 error channel。

H3/D14 的验证分层也反映这些边界。Reference 检查完整教学域：非阻塞 task-as-sender、env/scheduler/live stop、terminal 销毁 op、无需移动赋值的 receiver 与不可移动 operation state。独立 good 基于固定 `exec::basic_task` 的独立机制：自定义 context 桥接 `get_scheduler/get_start_scheduler`，boxed value 避免 `exception_ptr` value 与 error 表示碰撞，并通过真实 `stdexec::connect(inner, bridgeReceiver)` 驱动，不使用 detached thread。具体执行结果和工具链边界属于本机验证记录。

## 把状态图落实到关键代码

```text
创建 task：frame 已分配，initial_suspend 暂停
  → start 或父协程 co_await：安装环境及终结路径
  → sender await：connect 得到稳定 op，start 可能同步完成或真正挂起
  → completion：先存储通道，再恢复协程
  → co_return / unhandled_exception：promise 保存最终结果
  → final_suspend：捕获续接目标，发布完成，转移控制权
  → 拥有者收束后销毁 frame
```

同步完成是最容易误判的路径。调用 `start(op)` 的栈帧还存在，receiver 已经可能恢复 coroutine，销毁 awaiter 甚至外层 operation。因而 await_suspend 的最后一次动作可以是启动操作，之后不要再读 this/op；不能在已经恢复一次之后再返回会导致额外恢复的结果。异步完成则必须由外部完成源保证操作及借用buffer仍存活。

发布完成后的代码尤其短，但顺序不可交换：

```cpp
auto next = promise.continuation;
auto terminal = promise.completion;
auto state = promise.completion_state;
{
    std::lock_guard lock(promise.mutex);
    promise.done = true;
    promise.cv.notify_all();
}
// 外部 waiter 此时可以返回并释放 frame；仅使用事先保存的局部值。
if (terminal) {
    terminal(state, handle);
    return std::noop_coroutine();
}
return next ? next : std::noop_coroutine();
```

这里有互斥的两种消费入口：sync_wait 拥有者等待 done；sender 入口由 terminal 把结果交给 receiver。terminal 调用前 frame 必须仍由对应 operation 持有；回调可释放该 operation，因此回调之后也不再读取 promise。子 task 的续接使用返回 coroutine_handle 的对称转移，避免把父子恢复简单叠成递归调用。

完成参数的类型也要从正确环境推导。H3用 `value_types_of_t<Sender, PromiseEnv, tuple, variant>` 得到当前promise环境里的 value 形状，支持无value或一个拥有值；不能用 `decltype(sync_wait(sender))` 代替，因为后者查询的是 sync_wait 环境。结果存储按 variant index 区分value/error/stopped，`exception_ptr` 普通值不能因类型相同误入错误通道。

Receiver 的要求与 operation 不同：协议需要 receiver 能进行不抛异常的移动构造，不要求移动赋值。外层 operation 则需要稳定地址并且不可移动。H3 `sender_op` 直接拥有 coroutine handle，既明确析构责任，也避免在 `task<void>` 尚未完成定义时按值嵌入自身类型。

独立good用 `exec::basic_task` 的context扩展和 `stdexec::connect` 的awaitable适配完成同样的任务。context把本题get_scheduler语义映射到固定库的get_start_scheduler；结果用value_box承载，保证exception_ptr业务值和库内部错误载荷具有不同类型。这些差异让good成为另一条验证依据，完整checker不因使用good而跳过环境、live stop、scheduler或异常值测试。
