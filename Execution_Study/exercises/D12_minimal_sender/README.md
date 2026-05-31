# 练习 12：最小 sender 与 operation_state

## 目标

手写一个最小 sender，让你亲眼看到：sender 是蓝图，`operation_state` 才是一次具体执行实例。**这题的 `completion_signatures` 是必做内容。**

## 前置理解

- 你已经完成练习 11，能够手动 `connect` 和 `start`。
- 你知道 sender 至少要能与 receiver 连接。
- 你知道 sender 需要声明 `completion_signatures` 才能与标准组合器互操作。

## 必做任务

1. 设计一个最小 sender，例如 `single_value_sender`，内部只保存一个整数或一个小结构体。
2. 为它定义 `connect(receiver)`，返回一个你自己写的 `operation_state` 类型。
3. 这个 `operation_state` 至少要保存两样东西：
   - 要发送的值
   - 被连接的 receiver
4. 为 `operation_state` 实现 `start()`，在其中调用 `set_value(receiver, value)`。
5. 用上一题的 logging receiver 手动连接并启动它。
6. **为 sender 定义 `completion_signatures`**，至少包含 `set_value_t(int)` 和 `set_error_t(std::exception_ptr)`。
7. **验证你的 sender 能被 `sync_wait` 消费**。如果 `sync_wait` 编译不过，检查 `completion_signatures` 是否正确——这就是签名声明的实际作用。
8. 在笔记里明确写出：sender、receiver、operation_state、completion_signatures 各自承担什么责任。

## 进阶任务

- 把 sender 从 value-only 版本升级为可配置模式：根据一个标志决定发 value、error 或 stopped。相应更新 `completion_signatures` 声明。
- 尝试故意写错 completion_signatures（声明 `set_value_t(int)` 但实际发 `set_value_t(string)`），观察编译器给你什么提示。
- 如果你愿意挑战，再试着让它和 `then` 组合，观察"进入通用生态"需要补哪些元信息。

## 验收点

- 你能清楚说出 sender 和 operation_state 的职责分界。
- 你能手动触发一次 value completion，并看到 receiver 被调用。
- 你没有把 receiver 以悬空引用等危险方式塞进 operation_state。
- 你能解释为什么 `connect` 不能直接返回"立刻运行完的结果"。
- **你的 sender 能被 `sync_wait` 消费，证明 `completion_signatures` 声明正确。**

## 常见坑

- 在 sender 内部直接调用 receiver，完全跳过 operation_state。
- `operation_state` 没有稳定拥有 receiver，导致生命周期不安全。
- `start()` 里重复发射 completion，破坏一次执行的基本语义。
- 一开始就追求支持所有概念，结果主线对象关系反而没看清。
- 忘记定义 `completion_signatures`，导致 `sync_wait` 编译失败后以为是"库 bug"。

## 对应官方参考

- P3090R0 的基础对象关系说明
- P3143R0 对示例的分层拆解
