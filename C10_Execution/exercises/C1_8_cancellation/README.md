# C1_8 取消不是错误

本题把 stop request、合作检查、stopped completion 分开。`stop_token` 只表示请求；只有 operation 发出 `set_stopped()`，这次执行才真的走 stopped channel。

## 知识

取消是合作式的。运行中的工作可以定期检查 token，看到请求后停止；也可以忽略请求继续完成。请求本身不是异常，也不自动销毁线程或 operation state。

完成后还有一条硬规则：发出 `set_value/set_error/set_stopped` 后，下游 receiver 可能销毁整条 operation state。发送方不能再读写自身状态。

## 机制

Reference 用受控步骤，不靠 `sleep` 猜调度：before start 一进入就请求 stop；during work 在第 2 步请求 stop；after completion 先发 `set_value(3)`，`start` 返回后外层再请求 stop；error 单独发 `set_error(exception_ptr)`。

bad 版本漏掉 during-work stopped，检查器拒绝。

## 必做

1. 保存一个 stop source/token。
2. 在 `start` 里实现受控步骤循环。
3. 分别覆盖 before/during/after 三个窗口。
4. 用 receiver 记录 value、error、stopped。
5. 确认 after-completion 请求不会改写已完成结果。

## 进阶

- 增加一个忽略 stop token 的 sender，观察它仍可正常 value completion。
- 增加 stop callback 日志，但不要把 callback 当 completion。

## 答案解释

before/during 都在完成前观察到请求，所以发 stopped；after 在完成后才请求，不能碰 opstate，也不能把 value 改成 stopped。检查器的 after-completion 请求放在 `start` 返回后，避免用真实 UB 当实验。
