# 练习 G-1：实现 my_then

## 目标

亲手实现一个简化版 `then` adaptor，把 inner receiver 拦截值通道、应用变换函数、推导输出签名的完整模式刻进直觉。这是所有 sender adaptor 的原型结构。

## 前置理解

- 你已经在模块 D 中做过 `tap` adaptor，理解 inner receiver 包装的基本形状。
- `then` 与 `tap` 的核心区别：`then` 的变换函数 `f` 会改变值的类型，因此 completion_signatures 必须跟着变。
- error 和 stopped 通道需要原样透传给下游 receiver。
- 先只处理单个 `set_value_t(T)` 签名的简化情况。

## 必做任务

1. **定义 `my_then_sender<InnerSender, F>`**：持有 inner sender 和 f，声明 `sender_concept`，推导 `completion_signatures`，实现 `connect`。

2. **定义 `my_then_receiver<DownstreamReceiver, F>`**：持有下游 receiver 和 f，实现 `set_value`（调用 f 并转发）、`set_error`（透传）、`set_stopped`（透传）、`get_env`（转发）。

3. **定义 `my_then_operation_state`**：持有 inner op state，`start()` 委托给 inner op。

4. **实现工厂函数 `my_then(Sender, F)`**。

5. **验证**：`sync_wait(my_then(just(42), f))` 返回正确结果。

6. **验证值类型变换**：`my_then(just(42), to_string)` 产出 string。

## 进阶任务

- 支持 `f` 返回 `void` 的情况。
- 支持 inner sender 产出多参数值（如 `just(1, 2)`）。
- 让 `f` 可能抛异常时也反映到 completion_signatures 中。

## 验收点

- `sync_wait(my_then(just(42), f))` 能编译通过并返回正确结果。
- error 和 stopped 通道不被吞掉，原样到达下游。
- 你能清楚画出从 `connect` 到 `start` 到 `set_value` 的完整调用链。

## 观察点

- `then` 的核心只做一件事：在 value channel 上插入一层函数调用。但为了让这件事在类型系统里合法，你需要写出 sender、receiver、operation_state 三个类型外加签名推导。
- 这种"小功能、大骨架"的模式就是所有 sender adaptor 的通用结构。
- inner receiver 是这个模式的关键角色：它站在 inner sender 和 downstream receiver 之间，负责拦截、变换、转发。

## 对应官方参考

- `stdexec` 源码中 `then` 的实现
- P2300R10 中对 sender adaptor 协议的说明
