# L01 application event loop

对应正文：[01-application-event-loop](../../chapters/01-application-event-loop.md)。

学生只编辑 `student/solution.hpp`。实现 `PreviewController::runPreview`：

1. 拒绝空任务，不能进入 `exec()`。
2. 用局部 `QEventLoop` 和 `QTimer` 逐帧推进 `MediaWorkbench` 预览。
3. 正常结束、超时和拒绝必须返回不同 exit code。
4. 所有路径有限结束，不依赖外部窗口或人工点击。

`reference` 是课程实现，`good` 是独立实现，`bad` 同步跑完事件、没有真正进入 Qt event loop，检查器必须拒绝它。
