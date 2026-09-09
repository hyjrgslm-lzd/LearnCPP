# 09 模块 G：symmetric transfer 与高级 task

D 模块写出了能返回值的 `task`，E 模块拆开了 `co_await`，F 模块解释了 frame 的生命周期。现在开始把 task 变成协程基础设施：一个结果可以被多个消费者等待，多个 task 可以一起启动并收束，普通 `main()` 可以同步等待协程完成。

本模块不追求完整 P2300，也不追求生产级 lock-free。目标是把三个工程问题讲透：

- `shared_task<T>`：同一个一次性结果如何被多个协程等待。
- `when_all` / `when_any`：多个子 task 如何 fan-out，再按明确规则 fan-in。
- `sync_wait`：非协程线程如何启动 root task 一次，并等 final completion 通知。

这三件事共同要求你精确处理所有权、等待者、完成通知和异常通道。小的玩具 `while (!done) resume()` 在这里开始失效。

<a id="g1"></a>

## 一、单 owner task 的边界

模块 D 的 `lazy_task<T>` 是单 owner 模型。它只允许一个对象拥有协程帧，也只允许一个消费者最终移动取走结果。

```cpp
task<int> compute() {
    co_return 42;
}

task<int> t = compute();
int value = co_await std::move(t);
```

这个模型简单可靠：结果可以 move，帧销毁责任清楚。代价是不能让两个协程都 `co_await` 同一个 task。第一个消费者取走结果后，第二个消费者面对的是已经消费过的状态。

Capstone5 reference 明确把这些行为当契约：

- `task` 禁 copy，move 转移 owning handle。
- `start()` 或 `co_await` 后不得二次启动。
- 完成后的 `await_resume()` 只能消费一次。
- `sync_wait` 要求传入未启动 task。

这些约束让帧所有权和结果所有权可证明。需要共享时，用新的类型表达共享语义。

## 二、shared_task 需要结果缓存和共享状态

`shared_task<T>` 的目标是：一个 producer 执行一次，多个 awaiter 都能拿到同一个结果。

```cpp
auto shared = share(source());
auto a = [&]() -> task<int> { co_return co_await shared; };
auto b = [&]() -> task<int> { co_return co_await shared; };
auto both = co_await when_all(a(), b());
```

这里 `source()` 只能执行一次。它完成后，结果要缓存起来，供 `a` 和 `b` 各自读取。由于多个消费者都要读，`await_resume()` 返回值通常是拷贝。

最小结构可以这样理解：

```text
shared_task<T>
  -> shared_ptr<shared_state<T>>

shared_state<T>
  mutex
  started
  done
  optional<T> value
  exception_ptr error
  vector<coroutine_handle<>> waiters
  optional<task<T>> source
  runner task<void>
```

reference 选择 `shared_ptr<shared_state<T>>`。原因很直接：producer frame 最终会被销毁，但共享状态还要管理结果缓存、waiter 列表和 runner 生命周期。把共享状态放在独立控制块里，语义接近 `std::shared_ptr`。

## 三、shared_task 的等待流程

等待 `shared_task` 时，awaiter 先检查是否完成：

```text
co_await shared
  -> lock state
  -> done 已 true：不挂起，await_resume 直接读缓存
  -> done 为 false：登记当前 coroutine_handle 到 waiters
  -> 若 producer 还没启动，创建 runner 并 start
  -> 当前协程挂起
```

runner 的任务是等待原始 source：

```text
runner
  -> co_await source
  -> 成功：state.value = result
  -> 失败：state.error = current_exception
  -> done = true
  -> 取出 waiters
  -> resume 每个 waiter
```

这里的 mutex 保护 `started/done/value/error/waiters`。教学 reference 简单地用 vector 保存 waiters，并在完成后逐个 `resume()`。生产实现还要处理 awaiter 提前销毁、并发插入与完成同时发生、最后一个引用释放时 source 尚未完成等窗口。

G-1 的重点是把这几条边画出来：shared_task 持有 shared_state，shared_state 持有 source 和 runner，waiter handle 指向等待者协程，完成后所有 waiter 都从 `await_resume()` 读同一个缓存值。

<a id="g2"></a>

## 四、when_all 是 barrier

`when_all(a, b)` 的语义是：两个子 task 都启动；全部完成后，父协程恢复；若有异常，等全部分支收束后传播首个异常。

```cpp
auto [left, right] = co_await when_all(fetch_left(), fetch_right());
```

对照下面这种顺序代码：

```cpp
auto left = co_await fetch_left();
auto right = co_await fetch_right();
```

顺序代码要等 left 完成后才开始等待 right。`when_all` 要先 fan-out，确保两个分支都获得启动机会。

reference 的二元结构可以简化成：

```text
when_all_op
  mutex
  task<T1> a, task<T2> b
  task<void> left_runner, right_runner
  optional<T1> left_value
  optional<T2> right_value
  exception_ptr error
  coroutine_handle<> parent
  remaining = 2
  waiting
```

`await_suspend(parent)` 中创建两个 runner，并给 runner promise 设置完成回调。随后启动两个 runner。每个 runner `co_await` 自己的子 task，把结果或异常写进 operation。runner 完成时，`final_suspend` 调用 operation 的 `complete()`，`remaining` 递减。最后一个完成的 runner 返回 parent handle，恢复父协程。

## 五、when_all 的错误策略

本课程 reference 使用 fail-delay：

```text
任一分支失败
  -> 保存首个 exception_ptr
  -> 其他分支继续运行
  -> remaining 仍等到 0
  -> await_resume 中重新抛首个异常
```

这个策略的好处是生命周期清楚：operation 不会在某个 loser 还可能访问共享状态时提前销毁。缺点是失败后仍等待其他分支完成。

其他策略也常见：

- fail-fast request-stop：首个错误请求停止其他分支，但仍要等它们进入完成状态。
- aggregation：收集所有异常，最后抛聚合错误。
- first-error：首错立即恢复父协程，同时后台处理 loser 生命周期。这个策略实现复杂，容易留下悬空访问。

G-2 先做 fail-delay。等你能证明“两个分支先全部启动、最后一个完成才恢复父协程”后，再考虑变参和取消传播。

## 六、when_any 比看上去更难

`when_any(a, b)` 表面上是“谁先完成就返回谁”。reference 的真实契约更精确：

```text
首个成功 value 获胜
  -> 保存到 variant
  -> 请求 stop_source.request_stop()
  -> 仍等待所有分支收束
  -> await_resume 返回 winner

没有任何成功 value
  -> 所有分支都完成后
  -> 传播首个异常
```

它等待全部分支收束，是为了避免 loser 仍在运行时 operation 已经析构。它请求 stop，是为了让 loser 如果支持 `stop_token`，可以尽快退出。

测试中有一个关键场景：右侧先成功，stop callback 已触发，但左侧还没释放。父协程此时不能返回；必须等左侧看到取消并完成。这个场景把 “winner chosen” 和 “operation complete” 分成两个时刻。

所以写 14/Capstone5 时要避免把 `when_any` 描述成“第一个完成立即返回”。本课程选择的是首个成功 value 产生 winner，全部分支完成后收束返回。

<a id="g3"></a>

## 七、sync_wait 是同步入口

C++ `main` 不能写成协程。标准没有定义谁来创建 main 协程帧、谁在 `initial_suspend` 后恢复它、谁负责最终销毁它。因此普通程序需要一个同步入口来消费协程：

```cpp
auto result = sync_wait(compute());
```

正确的 `sync_wait` 使用完成通知。盲目循环恢复同一个 handle 的写法如下：

```cpp
while (!h.done()) h.resume(); // 只适合极窄玩具 awaiter
```

这个循环假设所有 awaiter 都由当前线程自驱、不会跨线程完成、不会在 `await_suspend` 返回前同步恢复、不会通过 symmetric transfer 转交控制。A3/B3/C2/G2/G3 中的真实 awaiter 都会打破这些假设。

reference 的 `sync_wait` 做法很小：

```text
sync_wait(task<T> t)
  -> 要求 t 未启动
  -> 创建 sync_wait_state { mutex, cv, done }
  -> 把 completion callback 塞进 promise
  -> t.start() 只调用一次
  -> 当前线程 cv.wait(done)
  -> final_suspend 调用 callback，设置 done 并 notify
  -> sync_wait 醒来，读 value 或 rethrow error
```

完成通知来自 `final_suspend` 后续路径。这样无论协程同步完成、跨线程完成，还是通过中间 task 链完成，root 的完成都会落到同一个通知点。

## 八、同步完成窗口

很多 awaiter 会在 `await_suspend` 内部立刻完成。例如 sender 的 `start()` 可能同步调用 `set_value`。这会产生一个窗口：

```text
await_suspend 开始
  -> connect/start
  -> sender 同步 set_value
  -> 尝试 resume 当前协程
  -> await_suspend 还没返回
```

如果 awaiter 无条件保存 handle 后返回 `true`，当前协程可能已经完成又被标记为挂起。Capstone5 的 stdexec awaitable reference 使用 atomic phase：

```text
starting -> start operation
completion: exchange completed
await_suspend: compare_exchange starting -> suspended
  成功：返回 true，等待异步完成恢复
  失败：说明已同步完成，返回 false，当前协程立即继续
```

G 模块不要求你实现 stdexec 桥接，但要把这种同步完成窗口记住。它也是 `when_all/when_any/sync_wait` reference 反复覆盖的原因。

## 九、G-1：实现 shared_task

进入 [练习 G-1](exercises/G1_shared_task/README.md) 时，先按对象关系读代码：

```text
shared_task copy
  -> 共享同一个 state

first awaiter
  -> 登记 waiter
  -> 启动 producer runner

second awaiter
  -> 登记 waiter
  -> 不再重复启动 producer

producer 完成
  -> 缓存 value/error
  -> 唤醒两个 waiter
```

你要证明 producer 只执行一次，两个 awaiter 都拿到值，并且没有 handle 被 resume 两次。若你用 intrusive list，awaiter 节点通常在等待者协程帧里；若用 vector 保存 handle，重点是完成路径取出列表后统一恢复。reference 采用后者，代码更短，足够教学。

## 十、G-2：实现 when_all

[练习 G-2](exercises/G2_when_all_impl/README.md) 的 starter 中有顺序 drain 的味道。改造目标是 fan-out/fan-in：

```text
parent co_await when_all_2(a, b)
  -> await_suspend 保存 parent
  -> 创建 left/right runner
  -> 同时 start 两个 runner
  -> parent 挂起
  -> left 完成 remaining: 2 -> 1
  -> right 完成 remaining: 1 -> 0，返回 parent
  -> parent await_resume 取 tuple 或抛首个异常
```

reference 用 latch/barrier 断言两个分支先全部启动，并且事件释放前都未完成。这个测试比 sleep 可靠，因为它直接控制完成顺序。

## 十一、G-3：实现 sync_wait

[练习 G-3](exercises/G3_sync_wait_impl/README.md) 的最终口径要以 reference 为准：`sync_wait` 是 root start once + final completion notification；blind-resume 循环只作为受控观察工具。

练习里可以保留手动驱动版本作为反例和观察工具，但正文结论必须明确：

- 对只含 `suspend_always` 且完全由当前线程恢复的 toy coroutine，循环 resume 能演示状态机。
- 对真实异步 task，循环 resume 会早读结果、重复恢复或碰到跨线程完成窗口。
- 生产形状应使用 condvar/run_loop/receiver，把完成信号转成阻塞等待。

Reference 还验证：`task<void>`、异常重抛、跨线程完成、同步完成、重复 `start()` 拒绝、frame 最终销毁。写笔记时至少覆盖这些观察项。

## 本模块完成后应能说清楚

完成 G-1 到 G-3 后，你应该能回答：

1. 为什么单 owner `task<T>` 不能被多个消费者共享。

   **答案解析：** 单 owner `task<T>` 拥有一个 coroutine frame，结果通常在 `await_resume()` 中被 move 出来并标记消费。两个消费者同时等待会让 continuation、启动状态和结果所有权互相覆盖；第一个消费者取走结果后，第二个消费者只能看到已消费状态。Capstone5 reference 因此禁 copy，并拒绝重复启动和重复消费。
2. `shared_task<T>` 为什么需要独立 shared state 和结果缓存。

   **答案解析：** producer frame 只应执行一次，但多个 awaiter 都要在不同时间读取同一个结果。独立 `shared_state` 用 `shared_ptr` 延长缓存、waiter 列表和 runner 的寿命，避免 producer frame 销毁后结果入口消失。G-1/reference 的 state 保存 `started/done/value/error/waiters/source/runner`。
3. 多 waiter 唤醒时如何避免重复 resume。

   **答案解析：** 等待者登记到同一个受同步保护的列表，producer 完成后设置 `done=true`，取出列表并逐个恢复。完成路径取走 waiters 后列表清空，后续 awaiter 看到 done 直接走 ready 路径，不再入队。G-1 教学实现用 vector 保存 handle，reference 用 mutex 保护完成与登记的交界。
4. `when_all` 为什么是 barrier，为什么要先启动全部分支。

   **答案解析：** `when_all` 的语义是所有分支完成后才恢复父协程，所以它需要一个 remaining 计数作为 barrier。先启动全部分支能保证 fan-out；若先完整等待 left 再启动 right，只是顺序组合。G-2 reference 用 latch/barrier 证明两个分支先都启动，最后一个完成才恢复 parent。
5. fail-delay 为什么等所有分支完成后才传播异常。

   **答案解析：** 某个分支失败时，其他分支仍可能访问 operation 的结果槽、计数器或取消状态。fail-delay 保存首个 `exception_ptr`，等 remaining 到 0 后在 `await_resume()` 重抛，保证 loser 已收束、共享状态还活着。G-2 的 error 场景验证异常传播时两个分支都已完成。
6. `when_any` 为什么选出 winner 后仍要收束 loser。

   **答案解析：** winner 只表示第一个成功 value 已写入 variant，不表示 operation 可以析构。loser 可能仍在运行并持有指向 operation 的回调或状态指针，所以 reference 先 `request_stop()` 帮助 loser 尽快退出，再等两个 runner 都完成后返回。`when_any_test.cpp` 特意把 winner chosen 和 loser release 分开验证。
7. `sync_wait` 为什么只 start root 一次，完成通知为什么要来自 final path。

   **答案解析：** root task 启动后，后续恢复可能由子 task、线程、事件循环或 symmetric transfer 推进；同步线程不能盲目循环 `resume()` 同一 handle。把 completion callback 接到 root promise 的 `final_suspend` 后，所有正常返回、异常和嵌套完成都会汇合到同一通知点。G-3/reference 用 root start once、nested completion、异常重抛和跨线程完成覆盖这条契约。
8. 同步完成发生在 `await_suspend` 返回前时，awaiter 要怎样避免错挂起。

   **答案解析：** awaiter 需要用同步状态把“正在 start”“已挂起”“已完成”区分开。若 completion 先发生，`await_suspend` 应返回 false，让当前协程立即继续；若先成功登记为 suspended，completion 负责 resume caller。Capstone5 的 stdexec bridge 用 atomic phase 做这件事，并用 abandoned 状态处理 waiter 提前销毁。

继续阅读：[14 第三阶段结课：mini 协程库实现](14-第三阶段结课-mini协程库实现.md)。
