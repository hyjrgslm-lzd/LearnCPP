# 14 第三阶段结课：mini 协程库实现

模块 D 到 G 已经把协程基础设施的关键机制拆开：promise 创建返回对象，`co_await` 查找 awaiter，协程帧保存跨挂起状态，`final_suspend` 通知等待者，组合器把多个 task 收束成一个结果。本章把这些机制重新合在一起，做一个小而完整的 mini 协程库。

项目稳定 ID 是 `Capstone5_mini_corolib`。数字 5 是结课项目 ID，不随章节重排改变。学生代码位于：

```text
Coroutine_Study/exercises/Capstone5_mini_corolib/include/mini
Coroutine_Study/exercises/Capstone5_mini_corolib/src
Coroutine_Study/exercises/Capstone5_mini_corolib/tests
```

Reference 位于：

```text
Coroutine_Study/exercises/Capstone5_mini_corolib/reference
```

本章讲的是当前仓库这套 reference 的真实契约。它是教学用 mini 库，范围小于完整 P2300 实现。核心路径只用 C++ 标准库；stdexec sender 桥接是可选扩展，只在第三方 target 可用时构建。

## 一、这个项目要把哪些机制合起来

先把 D-G 的机制映射到组件：

| 已学机制 | mini 组件 | 组件职责 |
| --- | --- | --- |
| promise 创建、lazy 启动、final suspend | `mini::task<T>` | 表示一次异步结果，单 owner，单次启动，单次消费 |
| `co_yield` 与 input range | `mini::generator<T>` | 同步惰性产生序列 |
| 共享结果缓存、多 waiter | `mini::shared_task<T>` | 一个 producer 执行一次，多个 awaiter 读同一结果 |
| barrier 与结果 tuple | `mini::when_all` | 两个子 task 都完成后返回 tuple，异常延迟传播 |
| winner 与 loser 收束 | `mini::when_any` | 首个成功 value 获胜，请求停止，全部分支收束后返回 |
| final completion -> condvar | `mini::sync_wait` | 普通线程同步等待 task 完成 |
| handle 队列 | `mini::run_loop` / `single_thread_executor` | 保存待恢复协程，按队列推进 |
| 协作停止 | `mini::stop_token` | 把停止请求传给支持取消的操作 |
| sender 到 awaiter | `as_awaitable` | 可选 stdexec 桥接，处理同步完成窗口 |

写代码时按这个顺序推进。每一层都有可运行观察；不要全写完再一起猜哪里坏。

## 二、核心约束：task 是单 owner、单次启动、单次消费

`task<T>` 是本项目的根。它持有一个 `std::coroutine_handle<promise_type>`，拥有对应协程帧。创建时停在 `initial_suspend`，调用 `start()` 或被 `co_await` 时才启动。

Reference 的 `task<T>` 有几个明确约束：

- 禁 copy，move 只移动 owning handle。
- `start()` 和 `co_await` 都会把 promise 标记为已启动。
- 已启动 task 再次 `start()` 会抛 `logic_error`。
- 结果只能在完成后消费一次。
- 异常保存在 promise 中，`await_resume()` 或 `sync_wait()` 重新抛出。

这些约束让所有权清楚。协程帧不会因为 wrapper move 而移动；移动的只是 handle 值。真正危险的是 start 之后，continuation、completion callback、operation state 和等待者关系已经建立，再让多个对象认为自己能控制同一帧。教学 reference 用运行时标记拒绝重复启动和重复消费。

对象关系如下：

```text
task<T> owner
  -> coroutine_handle<promise_type>
       -> promise
            optional<T> value
            exception_ptr error
            coroutine_handle<> continuation
            completion callback for sync_wait/combiner
            started/consumed flags
```

## 三、`final_suspend` 是所有完成路径的汇合点

`task` 的 promise 在 `final_suspend` 中决定完成后通知谁：

```text
task body finishes
  -> return_value 或 unhandled_exception 保存结果
  -> final_suspend.await_suspend
      -> 若有 sync_wait/combiner callback，调用 callback
      -> 否则若有 continuation，返回 continuation
      -> 否则返回 noop_coroutine
```

Reference 有一个细节：完成回调可能让另一个线程随后销毁这个已经挂起的 frame，所以 `final_suspend` 在发布完成前先读出 callback、state 和 continuation。写教材和注释时要保留这个因果关系：完成通知之后，当前协程帧的 owner 可能继续推进销毁。

这也是 `sync_wait`、`when_all`、`when_any` 都接到 final path 的原因。它们不自己循环 resume root task；它们等待 root task 的完成信号。

## 四、`sync_wait` 把协程完成翻译成线程唤醒

`sync_wait(task<T>)` 是普通 `main()` 使用协程的入口。它要求 task 尚未启动，然后：

```text
创建 sync_wait_state { mutex, condition_variable, done=false }
把 state 和 completion callback 写入 promise
调用 t.start() 一次
当前线程 wait(done)
final_suspend 调用 callback
callback 加锁设置 done=true 并 notify_one
sync_wait 醒来，读取 value 或重新抛出 error
```

返回类型是：

```cpp
std::optional<std::tuple<T>>
std::optional<std::tuple<>>
```

这和 P2300 `sync_wait` 的“可能 stopped”形状相近。当前 core reference 的 `task` 没有 stopped 完成通道，所以成功时返回有值 optional，错误时抛异常。

测试覆盖的关键场景：

- `task<int>` 返回值。
- `task<void>` 完成。
- `task<int>` 和 `task<void>` 异常重抛。
- awaiter 在 `await_suspend` 返回前跨线程恢复。
- awaiter 在 release 后恢复。
- root task 只启动一次。
- frame 中的 RAII 对象最终析构。

因此实现 `sync_wait` 时，不要用 `while (!h.done()) h.resume()`。这个循环只适合完全自驱的演示 awaiter，无法覆盖跨线程和同步完成窗口。

## 五、`generator<T>` 是同步惰性序列

`generator<T>` 使用 `co_yield` 交付中间值。它的 promise 保存当前值：

```text
begin()
  -> resume 到第一个 co_yield
  -> promise.current = value
  -> iterator 解引用读取 current

++iterator
  -> resume 到下一个 co_yield 或 final_suspend
```

它是 input range。对同一个 generator 对象，消费会推进同一个生产过程。若要多次遍历结果，把值存进容器；若要重新生成，重新调用 generator 函数。

实现时要注意 current 的生命周期。Reference 用 `std::optional<T>` 按值保存当前产出。这样消费者拿到的是 frame 内当前值的引用，下一次 `resume()` 可能覆盖它；generator owner 必须覆盖 iterator 使用期。

## 六、`shared_task<T>` 把单次结果变成多次读取

`shared_task<T>` 通过独立 shared state 解决两个问题：结果缓存和多 waiter。

```text
shared_task<T>
  -> shared_ptr<shared_state<T>>

shared_state<T>
  mutex
  started/done
  optional<T> value
  exception_ptr error
  vector<coroutine_handle<>> waiters
  optional<task<T>> source
  shared_ptr<task<void>> runner
```

第一次 await 时，state 登记 waiter，并启动 runner。runner `co_await source`，把结果或异常存进 state，再唤醒所有 waiter。后续 await 如果 state 已 done，直接走 ready 路径，从缓存读结果。

`await_resume()` 返回拷贝。共享语义要求每个消费者都能读到值；若第一个消费者 move 走结果，第二个消费者就只能看到被移动后的对象。

当前 reference 采用 mutex + vector，足够说明教学目标。生产级 `shared_task` 还要处理 waiter 协程提前销毁、并发插入与完成同时发生、最后引用释放时 source 未完成等更难窗口。

## 七、`when_all` 是二元 barrier

当前 reference 实现二元 `when_all(task<T1>, task<T2>)`，返回 `task<std::tuple<T1, T2>>`。它的完成规则：

```text
await_suspend(parent)
  -> 创建 left/right runner
  -> 给两个 runner 设置 completion callback
  -> start left
  -> start right
  -> parent 挂起，除非两个 runner 已同步完成

runner 完成
  -> 保存自己的 value 或首个 error
  -> final_suspend callback: remaining--
  -> remaining 到 0 时返回 parent handle

await_resume()
  -> 有 error 则 rethrow
  -> 否则返回 tuple(left, right)
```

这里的 “all” 是收束语义。某个分支失败时，另一个分支仍会跑完，最后再传播首个异常。这样 operation 的结果槽和子 task 生命周期不会提前失效。

`when_all_result<task<int>, task<int>>::completion_signatures` 在 reference 中可查，证明组合器能在编译期声明自己的 value 类型。

## 八、`when_any` 是 winner 加收束

当前 reference 实现二元 `when_any(task<T1>, task<T2>, stop_source)`，返回 `task<std::variant<T1, T2>>`。它的核心规则：

```text
第一个成功 value
  -> emplace 到 winner variant
  -> request_stop()

失败分支
  -> 若还没有 winner，保存首个 exception_ptr

两个 runner 都完成后
  -> 若有 winner，返回 winner
  -> 若没有 winner，抛首个异常
```

这意味着 `when_any` 在选出成功值后仍等待 loser 收束。测试会故意让 winner 先完成并触发 stop，然后保持 loser 还没 release，确认父协程没有提前返回。等 loser 看到 stop 并完成后，整体才完成。

这个契约比“谁先完成就马上返回”更稳，因为 operation state 不会在 loser 仍访问它时析构。

## 九、run_loop、task_scope 和 stop_token

`run_loop` 是最小调度器：`schedule()` 返回 awaiter，`await_suspend` 把当前 handle 放进队列；`run_one()` 取出一个 handle 并恢复；`run()` 重复直到队列空。

```text
co_await loop.schedule()
  -> 当前协程 handle 入队
  -> 调用者稍后 loop.run_one()
  -> handle.resume()
```

`task_scope` 是结构化并发的教学版本。它接收 task，在线程中 `sync_wait`，维护 `in_flight` 计数，`wait_empty()` 等计数归零并 join 线程。析构会尝试等待完成并吞掉异常，显式 `wait_empty()` 会重新抛出保存的首个异常。

`stop_token` 当前直接使用 `std::stop_token` / `std::stop_source`。它在 `when_any` loser 场景中体现：winner 请求停止，支持 token 的 loser 在恢复后检查 `stop_requested()` 并尽快返回。

## 十、stdexec `as_awaitable` 是可选桥接层

核心 reference 不依赖 stdexec。`reference/include/mini_ref/stdexec_awaitable.hpp` 是可选桥接示例，用于说明 E 模块的 `await_transform/as_awaitable` 能怎样接 sender。

它的关键在同步完成握手：

```text
await_suspend(caller)
  -> connect(sender, receiver)
  -> start(operation)
  -> 保存 caller
  -> starting -> suspended 成功：返回 true
  -> 若 start 已同步完成：phase 已 completed，返回 false

receiver.set_value/error/stopped
  -> 保存结果
  -> phase exchange completed
  -> 若之前已 suspended，resume caller
```

这里的 operation state 放在 shared state 里，保证 sender 完成前仍存活。awaiter 析构时若协程被放弃，会把 phase 标为 abandoned，避免稍后恢复已销毁的 caller。

本章保留 H 模块的 sender 桥接入口，但不要把它写成 Capstone5 core 的必需依赖。核心 9 个 reference target 应该在没有 stdexec 时照常构建。

## 十一、建议实现顺序

按下面顺序实现，哪里失败就只修当前层：

1. `task<T>` / `task<void>`：value、error、continuation、start once、consume once。
2. `sync_wait`：final completion callback + condvar。
3. `generator<T>`：`yield_value` + input iterator。
4. `run_loop`：队列保存 handle。
5. `when_all`：二元 task barrier，全部启动，全部收束。
6. `when_any`：winner variant，request_stop，全部收束。
7. `shared_task`：shared state、runner、waiters、缓存结果。
8. `task_scope` / `stop_token`：结构化等待和协作停止。
9. 可选 `stdexec_awaitable`：同步完成、错误、stopped、abandoned。

每层的验证都已有 reference test。学生 starter 可以先补最小 int 版本，再泛化到模板。不要新增第三方依赖；核心路径只用标准库。

## 十二、Reference 验收入口

核心 reference target：

```powershell
cmake -S Coroutine_Study/exercises --preset verify-core
cmake --build Coroutine_Study/exercises/build/verify-core --config Release --target mini_reference_task mini_reference_generator mini_reference_sync_wait mini_reference_when_all mini_reference_when_any mini_reference_run_loop mini_reference_task_scope mini_reference_stop mini_reference_shared_task
ctest --test-dir Coroutine_Study/exercises/build/verify-core -C Release -R "mini_reference_"
```

stdexec 桥接 target 只在 `stdexec::stdexec` 可用时构建。它验证 value、error、stopped、同步完成和 abandoned waiter 窗口。缺少 stdexec 时，核心 mini 库验收不应失败。

## 本项目完成后应能说清楚

完成 `Capstone5_mini_corolib` 后，你应该能回答：

1. `task` 的创建、启动、完成、消费、销毁各发生在哪个对象上。

   **答案解析：** 创建时协程函数构造 promise，并由 `get_return_object()` 返回持有 `coroutine_handle<promise_type>` 的 `task` owner。启动发生在 `task::start()` 或 `task::await_suspend()`，完成时协程体把 value/error 存进 promise 并进入 `final_suspend`。消费发生在 `await_resume()` 或 `sync_wait()` 醒来后读取 promise，销毁由 `task` owner 析构调用 `destroy()` 完成。
2. `sync_wait` 怎样把 final completion 转成线程阻塞唤醒。

   **答案解析：** `sync_wait` 创建带 mutex/cv/done 的 state，把 state 指针和 completion callback 写进 root promise，然后只启动 root 一次。root 走到 `final_suspend` 时 callback 设置 done 并 `notify_one()`，阻塞线程醒来后读取 value 或重抛 error。reference 的 `detail::sync_wait_state::complete` 就是这个回调入口。
3. `generator` 的当前值为什么属于协程帧，iterator 为什么不能比 generator owner 长寿。

   **答案解析：** `co_yield` 会把当前产出保存到 generator promise 的 `current` 中，iterator 解引用读的是 frame 内这个当前值。下一次 `resume()` 可能覆盖它，generator owner 析构会 `destroy()` frame。iterator 只保存 handle，没有所有权，所以不能超过 generator 对象寿命。
4. `shared_task` 为什么要缓存结果并返回拷贝。

   **答案解析：** `shared_task` 的目标是一个 producer 执行一次，多个 awaiter 都能看到同一结果。结果放在独立 shared state 中，完成后 waiter 和后续 awaiter 都从缓存读取；返回拷贝可以避免第一个消费者 move 走值后破坏第二个消费者。reference 的 `shared_task<T>::awaiter::await_resume()` 返回 `*state->value`。
5. `when_all` 为什么是 barrier，错误为什么延迟到全部分支收束后传播。

   **答案解析：** `when_all` 必须等两个 runner 都完成，才能保证结果槽、错误槽和子 task 生命周期都稳定。任一分支失败时先保存首个异常，remaining 仍等到 0，最后在 `await_resume()` 重抛。reference 的 `when_all_op::complete` 只有 `remaining_` 减到 0 且 parent 正在等待时才返回 parent handle。
6. `when_any` 为什么 winner 选出后仍要等待 loser 完成。

   **答案解析：** winner 写入 variant 后，loser 仍可能持有 operation 内部状态并继续运行。reference 在 winner 产生后调用 `stop_.request_stop()`，让支持 stop token 的 loser 尽快退出，但仍等 remaining 到 0 才恢复 parent。这样避免父协程返回并析构 operation 时 loser 还在访问它。
7. `run_loop` 与 `sync_wait` 分别负责哪种推进方式。

   **答案解析：** `run_loop` 是协程内/事件循环式推进：`schedule()` 把 handle 入队，调用者用 `run_one()` 或 `run()` 恢复队列中的协程。`sync_wait` 是普通线程阻塞入口：启动一个 root task，然后等待 final completion 通知。前者负责调度待恢复 handle，后者负责把协程完成转成线程等待结果。
8. `stop_token` 在组合器里怎样帮助 loser 尽快退出。

   **答案解析：** `stop_token` 是协作取消信号，不会强杀协程或线程。`when_any` 选出成功 winner 后调用对应 `stop_source.request_stop()`，支持 token 的 loser 在恢复或检查点看到 `stop_requested()` 后提前返回。`when_any_test.cpp` 用 stop callback 和 loser release 验证 winner 后取消信号已经发出，但整体仍等 loser 收束。
9. stdexec bridge 为什么必须处理 `start()` 同步完成和 waiter 提前销毁。

   **答案解析：** sender 的 `start()` 允许同步调用 receiver 的 `set_value/set_error/set_stopped`，这可能发生在 `await_suspend` 返回前。bridge 若无 atomic phase，会把已经完成的 awaiter 错判为仍挂起，或稍后恢复已销毁 caller。reference 的 `stdexec_awaitable` 用 `starting/suspended/completed/abandoned` 区分这些窗口，并把 operation state 放在 shared state 中保活。
10. 哪些行为是标准语义，哪些只是本 reference 的教学选择，哪些属于目标编译器的实现观察。

   **答案解析：** 标准语义包括 promise 查找、initial/final suspend、`co_await` 三方法、`await_suspend` 返回类型含义、frame 需要保存跨挂起状态，以及允许但不保证 HALO。教学选择包括单 owner task、start/consume 运行时检查、二元 `when_all/when_any`、mutex+vector waiters、`sync_wait` 返回 optional tuple 和 core 不依赖 stdexec。实现观察包括 frame 字段顺序、具体 size、汇编形状、机器级 tail call 和某个工具链是否真的 elide allocation。

练习入口：[Capstone5_mini_corolib](exercises/Capstone5_mini_corolib/README.md)。
