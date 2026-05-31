# 练习 13：最小 sender adaptor

## 目标

手写一个最小 sender adaptor（包装另一个 sender 并变换其行为），理解 `then`、`upon_error` 等库算法的内部结构模式。

## 前置理解

- 你已经完成练习 12，能手写 sender、receiver、operation_state。
- 你理解 completion_signatures 的作用。
- 你接受这题先做一个只关注 value channel 的最小 adaptor。

## 必做任务

1. 设计一个 `tap` adaptor：它包装一个 inner sender，在 value completion 时先执行一个 side-effect 函数（如打印日志），然后把值原样转发给下游 receiver。
2. 实现 `tap_sender<InnerSender, F>` 类型：
   - 持有 inner sender 和 side-effect 函数 `f`
   - 定义 `completion_signatures`：与 inner sender 相同
   - 定义 `connect(receiver)`：创建一个 `tap_receiver` 包装下游 receiver
3. 实现 `tap_receiver<DownstreamReceiver, F>` 类型：
   - `set_value`：调用 `f(values...)`，然后调用下游 receiver 的 `set_value`
   - `set_error`：直接转发给下游 receiver
   - `set_stopped`：直接转发给下游 receiver
   - `get_env`：转发给下游 receiver
4. 实现 `tap_operation_state`：连接 inner sender 和 tap_receiver。
5. 验证：`sync_wait(tap(just(42), [](int x){ std::cout << "tap: " << x; }))` 应打印 "tap: 42" 并返回 42。
6. 在笔记中画出 sender 嵌套图：`tap_sender` 持有 `inner_sender`，`connect` 时生成 `tap_receiver` 包装 `downstream_receiver`，再用 `tap_receiver` 与 `inner_sender` connect。

## 进阶任务

- 再做一个 `map` adaptor（类似简化版 `then`）：与 `tap` 不同，`map` 的函数 `f` 改变值的类型，因此 `completion_signatures` 需要从 inner sender 的签名变换而来。
- 为 `tap` 或 `map` 添加管道语法 `operator|` 支持：`just(42) | tap(f) | sync_wait`。
- 尝试让 adaptor 也正确转发 environment query。

## 验收点

- 你能清楚说出 sender adaptor 的核心模式：inner receiver 包装 + channel 拦截 + 其余透传。
- 你的 adaptor 能与 `sync_wait` 和其他标准 sender 互操作。
- 你能解释为什么 adaptor 的 `completion_signatures` 依赖于 inner sender 的 signatures。
- 你能指出 adaptor 中 sender、receiver、operation_state 的嵌套拥有关系。

## 常见坑

- 在 `tap_receiver::set_value` 里忘了转发给下游 receiver，结果图断了。
- `set_error` 和 `set_stopped` 没有转发，导致 error/stopped 通道丢失。
- `get_env` 没有转发，导致下游 environment query 失效。
- 让 `tap_receiver` 持有下游 receiver 的引用而非值，导致生命周期问题。
- adaptor 的 `completion_signatures` 写错，与 inner sender 不一致。

## 对应官方参考

- `stdexec` 中 `then` 的实现思路
- P3090R0 中对 sender adaptor 组合的说明
