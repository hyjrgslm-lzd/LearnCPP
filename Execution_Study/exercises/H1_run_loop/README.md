# 练习 H-1：my_run_loop

## 目标

亲手实现一个最小的 run_loop 调度器，包含 intrusive FIFO queue、互斥与条件变量驱动的事件循环、以及一个返回 sender 的 `schedule()` 接口。通过这道题把"scheduler 到底是怎么把 operation_state 排队并执行的"刻进直觉。

## 前置理解

- 你已经完成模块 D 和模块 G，能手写 sender、receiver、operation_state。
- `operation_state` 在 sender-receiver 模型里必须 non-movable（一旦构造，地址不再改变）。
- intrusive data structure 的基本概念：节点自身携带链表指针，而不是把节点塞进外部容器。
- `std::mutex` 和 `std::condition_variable` 的基本用法。

## 必做任务

1. **定义 `operation_base` 基类**：包含 intrusive `next` 指针和纯虚函数 `execute()`。

2. **实现 intrusive FIFO queue**：`push_back`、`pop_front`、`empty`。

3. **实现 `my_run_loop` 类**：
   - `push(op)`: 加锁，入队，通知条件变量。
   - `run()`: 循环等待，出队后解锁，调用 `execute()`。finished 且 queue 空时退出。
   - `finish()`: 设置 finished，通知条件变量。

4. **实现 `schedule()` 方法**，返回一个 sender：
   - sender 的 connect 返回 `run_loop_operation_state<Receiver>`。
   - operation_state 继承 `operation_base`，`start()` 把自己入队，`execute()` 调用 `set_value`。

5. **验证**：用 `schedule()` 创建多个 sender，接 `then` 打印任务编号，用 `when_all` 汇合，在另一个线程运行 `loop.run()`。

## 进阶任务

- 在 `execute()` 中加入 try-catch，捕获异常后调用 `set_error`。
- 把 intrusive queue 替换为 LIFO（栈），观察执行顺序变化。
- 让 `my_run_loop` 支持 `get_scheduler` environment query。
- 把 `virtual execute()` 替换为函数指针，避免虚函数开销。

## 验收点

- `schedule()` 返回的 sender 能与 `then`、`when_all`、`sync_wait` 正常组合。
- 所有入队任务都在调用 `run()` 的那个线程上执行（单线程事件循环）。
- 你能画出 operation_state 入队、出队、execute 的完整生命周期。
- 你能解释为什么 `operation_state` 必须 non-movable：`start()` 之后地址已被 intrusive queue 持有。
- 你能解释 intrusive queue 和 `std::queue<std::function>` 的本质差异。

## 观察点

- `run_loop` 是标准里最简单的 scheduler 之一，因为它只有一个线程在消费 queue。
- intrusive data structure 在 sender-receiver 框架中无处不在，因为 operation_state 的 non-movable 特性天然提供了地址稳定性。
- `start()` 把 `this` 入队——这就是为什么 operation_state 不能被移动或复制。
- 这个模式也解释了为什么 `connect` 返回的 operation_state 必须由调用者保持存活。

## 对应官方参考

- P2300R10 中 `execution::run_loop` 的规范定义
- `stdexec` 中 `__run_loop` 的实现
- P3090R0 中对 operation_state 地址稳定性的说明
