# D13 最小 sender adaptor：tap

G1 的 `my_then` 改写 value 类型；本题写一个不同语义的 adaptor：`tap` 只观察 value，执行副作用，然后把原 value 原样转发。

## 知识

adaptor 的骨架是：外层 sender 保存 inner sender 和函数；connect 时创建 wrapper receiver；wrapper receiver 拦截自己关心的 channel；operation state 保存 inner connect 的结果；start 只启动 inner op。

`tap` 不改变 payload 类型，所以 value completion 应原样转发。error/stopped 不是 tap 的语义目标，必须透传。`get_env` 也必须透传，否则上游环境查询会断。

## 机制

检查器使用自写 sender 驱动四条路径：value sender 发送 `(2, 5)`，tap 记录一次副作用并原样转发；error sender 发送 `"boom"`；stopped sender 发送 stopped；env sender 从 receiver environment 读 marker，证明 wrapper receiver 转发 `get_env`。

bad 版本把 stopped 改成 value，检查器拒绝。

## 必做

1. 写 `tap_sender<InnerSender, F>`。
2. 写 `tap_receiver<DownstreamReceiver, F>`。
3. `set_value` 先调用 `F`，再转发原参数。
4. `set_error`、`set_stopped`、`get_env` 原样转发。
5. operation state 保存 `connect(inner, tap_receiver)`。
6. `start` `noexcept` 委托 inner op。

## 进阶

- 如果 `F` 抛异常，把异常转成 `set_error(exception_ptr)`。
- 再实现一个 `map`，对比它为什么需要改写 completion signatures。

## 答案解释

Reference 不调用 `stdexec::then` 冒充实现，而是实际写出 sender、receiver、operation state。它与 G1 衔接的是内部结构，不重复 `my_then` 的类型变换目标。
