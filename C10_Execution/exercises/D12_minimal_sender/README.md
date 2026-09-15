# D12 最小 sender 与 operation_state

sender 是蓝图；operation state 是一次 connect 产生的执行实例。本题手写一个 sender，验证每次 connect 都有独立状态，`start` 发出恰好一条 completion。

## 知识

`connect(sender, receiver)` 只建立状态，不运行工作。`start(op)` 才触发执行。operation state 通常不可移动，因为上游或调度器可能保存它的地址。

`start` 要 `noexcept`。如果启动阶段遇到可报告错误，应通过 `set_error` 发送，而不是抛出。

## 机制

`single_value_sender` 支持三种模式：value 发送 `set_value(int)`；error 发送 `set_error(exception_ptr)`；stopped 发送 `set_stopped()`。检查器手动连接 probe receiver，直接检查 member `connect`、不可移动 operation state、`start noexcept` 和三种 completion。

bad 版本把所有模式都伪造成 value，检查器在 error mode 拒绝。

## 必做

1. sender 保存值和 completion 模式。
2. 每次 `connect` 返回新的 operation state。
3. operation state 按值拥有 receiver。
4. 删除 operation state copy/move。
5. `start` 根据模式发送一条终结信号。

## 进阶

- 增加 start-twice 诊断。
- 试着把签名写错，观察组合器诊断。

## 答案解释

正确答案的核心是所有权：sender 可复制描述；operation state 独占一次执行；receiver 被移动进 completion。两个 connect 分别 start，应该得到两条独立 value 事件。
## IDE 工程入口

VS solution 中本题主入口是 `D12_minimal_sender_student`。学生只编辑 `src/student/solution.hpp`；`main.cpp` 是共同检查器，Reference 在 `src/reference/solution.hpp`，good/bad 控制在 `validation` 下。`D12_minimal_sender` 聚合目标只负责显式构建学生目标，收在 Support；Reference 与控制目标保留为独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
