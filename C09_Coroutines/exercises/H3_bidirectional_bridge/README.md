# H-3 双向桥接

知识讲解：[H3 对应章节](../../10-模块H-协程与sender_receiver桥接.md#h3)。

对应主讲义：`10-模块H-协程与sender_receiver桥接.md` 的 H-3。

这题让 `my_task<T>` 同时服务两种消费者：sender 消费者通过 `stdexec::connect/start` 启动它；协程消费者通过 `co_await my_task<T>` 等它。第三条路径是在 `my_task` 协程体内 `co_await stdexec::just(42)`，复用 H-1 的 sender -> awaitable 桥。

学生入口只改 `student.hpp`。`checks/main.cpp` 是验收 fixture：它固定三条不可由学生改写的协程体，分别测试 `my_task` 被 sender `connect/start` 消费、外层 task `co_await` 内层 task、task 内部 `co_await` checker private fixture 提供的 traced sender。checker 使用 31、11、40 这些变化输入，并要求值 31/22/41 与六个计数全为 1；计数不作为可写参数传给学生实现。删除内部 await、伪造返回值，或尝试改写 bridge sender trace，会因为 child/sender 计数没发生或 private fixture 边界被拒绝。`validation/good/student.hpp` 和 `validation/bad_constant/student.hpp` 用同一个 checker 构建，分别证明真实双向桥和坏实现不能通过。

三个链路要分开看：

```text
connect(my_task, receiver) -> op_state owns task + receiver -> start -> final_suspend -> set_value
outer co_await inner       -> task_awaiter saves continuation -> child final_suspend -> resume outer
my_task co_await sender    -> promise.await_transform(sender) -> H-1 bridge receiver -> await_resume
```

预测 reference 输出：sender path 返回 42；awaitable path 中 `inner()` 返回 10，`outer()` 返回 20；bridge path 中 `stdexec::just(42)` 被 H-1 bridge 转成 `co_await` 结果，最终返回 43。

**答案解析：** sender path 由 `connect(my_task, receiver)` 创建 operation state，`start` 后 task 在 `final_suspend` 对外部 receiver 调 `set_value(42)`。awaitable path 由 `promise_type::await_transform(my_task&&)` 返回 `task_awaiter`，子 task 结束时通过 continuation 恢复父协程，所以 `10 * 2 = 20`。bridge path 复用 H1 的 sender awaitable，把 `just(42)` 的 value channel 还原成协程体内的 `int`，再加一得到 43。

实现时保留两个出口：`external_receiver` 只服务 sender path，`continuation` 只服务 awaitable path。`final_suspend` 必须拿精确的 `std::coroutine_handle<promise_type>`，这样才能访问本 promise 并按真实完成状态发 channel。

运行：

```powershell
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build-h -G Ninja -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build C09_Coroutines/exercises/build-h --target H3_bidirectional_bridge_reference
ctest --test-dir C09_Coroutines/exercises/build-h -R H3_bidirectional_bridge_reference --output-on-failure
```

观察到 `sender=42 awaitable=20 bridge=43` 后，重点复读 `my_task::op_state::start`、`promise_type::final_suspend` 和 `promise_type::await_transform(my_task&&)`。

**答案解析：** `op_state::start` 是 sender 消费路径的启动点，它设置 `external_receiver` 并恢复 task。`final_suspend` 是两种消费路径的分叉点：有 external receiver 时发 completion channel，有 continuation 时把控制权还给父协程。`await_transform(my_task&&)` 把自家 task 限定为内部可 await，避免 stdexec 把 `my_task` 当普通 awaiter 而绕开 `connect`。
