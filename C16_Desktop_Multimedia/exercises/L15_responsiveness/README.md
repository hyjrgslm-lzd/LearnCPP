# L15 最新预览与事件积压

正文、各 Part 和完整解析见 [响应性章节](../../chapters/15-responsiveness.md)。只编辑 `student/solution.hpp`；接口是 `PreviewMailbox(callback)`、`submit(value)`、`close()` 和 `posted()`。

`checks.cpp` 实际调用所选实现，检查最后值、通知数、线程位置、关闭、销毁和回调重入。`per_event.cpp` 是原始正确基线；`coalesced.cpp` 使用同一 Reference mailbox 进行同口径复验。只承诺当前请求的最后预览，不承诺每次中间值都交付。

构建和独立进程采样命令见正文及 [构建说明](../BUILD_GUIDE.md)。不要用 bad 的错误结果参与性能比较。
