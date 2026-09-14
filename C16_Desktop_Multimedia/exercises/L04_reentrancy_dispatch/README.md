# L04 reentrancy dispatch

对应正文：[04-dispatch-reentrancy](../../chapters/04-dispatch-reentrancy.md)。

学生只编辑 `student/solution.hpp`。实现一个小型 dispatch queue：

1. `post` 保留 FIFO。
2. `drain` 处理当前批次。
3. handler 内再次 `drain` 不得重入当前 handler。
4. `clear` 只丢弃 pending，不中断当前正在运行的 handler。

`bad` 在 handler 中直接重入，导致 inner 早于 outer 的后半段执行。
