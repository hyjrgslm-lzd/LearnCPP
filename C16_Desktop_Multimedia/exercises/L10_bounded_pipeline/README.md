# L10 bounded pipeline

正文：[L10 有界管线、背压、欠载与排空](../../chapters/10-bounded-pipeline.md)。

学生只编辑 `student/solution.hpp`。已提供 `checks.cpp`、`reference/solution.hpp`、独立 `good/solution.hpp` 和故意错误的 `bad/solution.hpp`。

任务：

1. 实现有界 FIFO，`write` 只接受剩余容量并返回 accepted count。
2. 区分 underrun、正常 close/drain 后 EOF、cancel flush。
3. 保持 `write/read/close/cancel/eof` 线程安全。
4. 实现有限等待版 `write_wait/read_wait`，用 `mutex + condition_variable` 在受控 producer/consumer 交错中证明背压、释放容量和 close drain。

有效检查：单线程状态检查覆盖容量、部分写、underrun、EOF、flush cancel；线程观察让 producer 在容量 2 处阻塞，consumer 读出后 producer 完成。bad 会因无限增长、错误 EOF 和无等待语义被拒绝。
