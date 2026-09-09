# 练习 H-3：my_task<T> 协程实现

## 目标

实现一个最小的协程 task 类型 `my_task<T>`，它能用 `co_await` 消费任意 sender，并在内部通过 bridge receiver 打通 coroutine 与 sender-receiver 的桥接通道。通过这道题把"sender-receiver 和 coroutine 在结构上的精确对应关系"刻进直觉。

## 前置理解

- 你已经完成模块 D 的协程桥接练习，知道协程和 sender 图的表达差异。
- 你理解 C++20 coroutine 的基本机制：`promise_type`、`coroutine_handle`、`initial_suspend`、`final_suspend`、`co_await`、`co_return`。
- 你知道 `co_await expr` 会调用 `promise.await_transform(expr)` 来获取 awaitable。
- 你知道 awaitable 的三个接口：`await_ready()`、`await_suspend(handle)`、`await_resume()`。
- 你接受这题是整套练习包中概念密度最高的一道题。先把每个步骤独立理解，再看整体对应关系。

## 必做任务

1. 实现 `my_task<T>` 的基本骨架：
   - `my_task<T>` 持有一个 `std::coroutine_handle<promise_type>`。
   - `promise_type` 定义：
     - `get_return_object()`：返回 `my_task<T>{handle_from_promise}`。
     - `initial_suspend()`：返回 `std::suspend_always{}`（lazy 启动，协程创建后不立刻执行）。
     - `unhandled_exception()`：把 `std::current_exception()` 存入 promise。
     - `return_value(T value)`：把 value 存入 promise。
   - `my_task<T>` 的析构函数销毁 coroutine_handle。

2. 实现 `final_suspend` 续接机制：
   - `final_suspend()` 返回一个自定义 awaitable，不是 `suspend_always` 也不是 `suspend_never`。
   - 这个 awaitable 的 `await_suspend(coroutine_handle<promise_type> h)` 检查：如果有等待者（continuation），就恢复等待者；否则不做任何事。
   - 在 promise 中保存一个 `std::coroutine_handle<> continuation_{}`，在有人 `co_await` 这个 task 时设置。

3. 实现 `await_transform(Sender)` -- 这是整道题的核心：
   - 当协程体写 `auto result = co_await some_sender;` 时，编译器调用 `promise.await_transform(some_sender)`。
   - `await_transform` 返回一个 `sender_awaitable<Sender, T>` 对象。

4. 实现 `sender_awaitable<Sender, Promise>` 类型：
   - `await_ready()`：返回 `false`（总是挂起，等待 sender 完成）。
   - `await_suspend(coroutine_handle<Promise> h)`：
     - 构造一个 bridge receiver，持有 coroutine handle 和一个用于存储结果的位置。
     - 调用 `connect(sender, bridge_receiver)`，得到 operation_state。
     - 调用 `start(operation_state)`。
     - 注意：operation_state 必须存活到 sender 完成，因此它应该被存储在 `sender_awaitable` 或 promise 中。
   - `await_resume()`：从存储位置取出结果并返回；如果存储的是异常，则 rethrow。

5. 实现 bridge receiver：
   - `set_value(values...)`：把值存入共享存储位置，然后调用 `handle.resume()` 恢复协程。
   - `set_error(error)`：把异常存入共享存储位置，然后调用 `handle.resume()` 恢复协程。
   - `set_stopped()`：（最小版本可以先存一个特殊标记或抛异常，后续再优化。）
   - `get_env()`：最小版本先返回 `empty_env{}`。

6. 结果存储：
   - 使用 `std::variant<std::monostate, T, std::exception_ptr>` 作为共享存储类型。
   - `await_resume()` 检查 variant 的状态：如果是 T 则返回值，如果是 exception_ptr 则 rethrow。

7. 验证基本功能：
   ```cpp
   my_task<int> example() {
       int x = co_await stdexec::just(42);
       co_return x + 1;
   }
   ```
   - 运行这个协程并取出结果（可以用一个简单的 `sync_run` 辅助函数来驱动 task）。
   - 确认返回值为 43。
   - 再验证错误路径：`co_await` 一个会抛异常的 sender，确认异常正确传播。

8. 在笔记中画出完整的对应关系表：

   | sender-receiver 概念     | coroutine 对应              |
   |--------------------------|-----------------------------|
   | sender                   | awaitable (via await_transform) |
   | connect(sender, receiver)| await_suspend 中的 connect + start |
   | receiver::set_value      | bridge receiver -> resume -> await_resume 返回值 |
   | receiver::set_error      | bridge receiver -> resume -> await_resume rethrow |
   | operation_state          | sender_awaitable 内部持有的 op state |
   | completion               | coroutine 恢复              |

## 进阶任务

- 实现 promise 的 `get_env()`：让它返回一个包含当前 scheduler 的 environment。这样当协程内部 `co_await` 一个需要查询 scheduler 的 sender 时，scheduler 信息能从 promise 传播到 bridge receiver 的 environment 中。
- 让 `my_task<T>` 自身也能被其他协程 `co_await`：实现 `my_task<T>` 的 `operator co_await()` 或在 `my_task<T>` 上直接定义 awaitable 接口。这样可以实现协程的嵌套组合。
- 处理 `co_await` 一个返回 void 的 sender（`set_value_t()`）。需要特化 `sender_awaitable` 让 `await_resume()` 返回 void。
- 让 `my_task<T>` 同时可以作为 sender 使用：为它定义 `completion_signatures` 和 `connect`，使其既能被 `co_await` 也能被 `sync_wait` 消费。

## 验收点

- 你的 `my_task<int>` 能够 `co_await stdexec::just(42)` 并正确拿到 42。
- 错误路径正确：`co_await` 一个抛异常的 sender 时，异常在 `await_resume()` 中被 rethrow。
- 你能画出从 `co_await sender_expr` 到 `connect -> start -> set_value -> resume -> await_resume` 的完整调用链。
- 你能解释 operation_state 为什么必须在 `sender_awaitable` 中存活，而不能是临时变量。
- 你能指出 sender-receiver 和 coroutine 之间的结构对应关系，而不只是笼统地说"它们都是异步的"。

## 观察点

- `await_transform` 是 coroutine 和 sender-receiver 之间的翻译层：它把"sender 语言"翻译成"coroutine 语言"。
- bridge receiver 是整个桥接的枢纽：它接收 sender 的 completion，然后恢复被挂起的协程。
- `co_await` 一个 sender 的过程本质上是：构造 bridge receiver -> connect -> start -> 挂起协程 -> sender 完成时 bridge receiver 恢复协程 -> await_resume 取出结果。
- 这和手动写 `connect(sender, my_receiver)` + `start(op)` 在结构上完全同构，只是协程语法把挂起/恢复的样板隐藏了。
- promise 的 `get_env()` 是 environment 从协程向 sender 传播的通道 -- 这就是为什么协程可以"感知"它所运行的执行上下文。

## 常见坑

- `sender_awaitable` 中的 operation_state 是临时变量，sender 完成前就被析构了。operation_state 必须存活到 completion 为止。
- bridge receiver 持有 coroutine_handle 的拷贝，但协程已经被销毁了 -- 检查 final_suspend 的实现是否正确。
- `await_suspend` 中先调用 `start(op)`，但 sender 同步完成导致 coroutine 在 `await_suspend` 返回前就被 resume 了。这种情况下 `await_suspend` 应该返回 `void` 或返回一个新的 handle（而不是 `bool`）来避免 use-after-suspend。
- `return_value` 和 `unhandled_exception` 同时存储了值，导致 variant 状态不一致。
- `final_suspend` 返回 `suspend_never`，导致协程在最后自动销毁，等待者拿到的 handle 已悬空。
- 忘记在 `my_task` 析构函数中 destroy coroutine handle，导致内存泄漏。

## 提示

- 先实现一个只支持 `co_return value` 的最小 task（不支持 `co_await sender`），确认协程基本骨架正确。
- 再加入 `await_transform` 和 bridge receiver，一步一步验证。
- operation_state 的存储位置是这题最棘手的设计决策。一种实用方案：用 `std::optional` 或对齐存储把 operation_state 放在 `sender_awaitable` 内部，但要注意 `sender_awaitable` 本身不能被移动。
- 如果你遇到"sender 同步完成"的问题，先假设所有 sender 都是异步完成的，把同步完成的边界情况留到进阶任务。
- 这道题的代码量大约在 120-150 行。如果你发现超过 250 行，检查是否过度抽象了。
- 画调用链图时，把协程挂起点和 sender completion 点用不同颜色标注。

## 复盘问题

- 为什么 `await_transform` 是把 sender 纳入协程生态的关键入口，而不是直接让 sender 实现 awaitable 接口？
- bridge receiver 的 `set_value` 恢复协程这一步，和普通 receiver 的 `set_value` 推动下一个 sender 那一步，在结构上有什么对应关系？
- 如果 sender 是同步完成的（`start()` 中直接调用 `set_value`），`await_suspend` 的控制流会出什么问题？标准是怎么解决的？
- 为什么说 coroutine 的 promise 可以看作一个"内置的 receiver"？
- 如果你想让 `my_task<T>` 既能被 `co_await` 又能被 `sync_wait` 消费，需要为它补什么接口？这说明 task 在概念上是什么身份？

## 对应官方参考

- P2300R10 中 `execution::as_awaitable` 和 `execution::with_awaitable_senders` 的规范
- `stdexec` 中 `task.hpp` 的实现
- `stdexec` 中 `__connect_awaitable` 的桥接逻辑
- Lewis Baker 的 `cppcoro::task` 实现（用于对比协程 task 的基础形状）
