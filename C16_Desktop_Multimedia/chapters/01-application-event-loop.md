# 01 应用对象与事件循环

MediaWorkbench 的第一个可运行版本不是播放器，而是一个本地审阅器：打开一个媒体条目，按事件推进预览状态，有限结束。桌面应用和命令行程序最大的差异在这里。命令行程序通常按调用栈推进；Qt 应用把输入、计时器、窗口系统消息、socket、跨线程投递都放进事件队列，由 `QCoreApplication::exec()` 或局部 `QEventLoop::exec()` 取出并分发。

事件循环解决的问题是“等待但不阻塞应用”。如果在主线程里直接 `while` 等待帧、等待文件、等待 worker，窗口不会响应，queued signal 也没有机会执行。反过来，如果随手开启嵌套事件循环而没有退出条件，同一个对象可能在你以为“当前函数还没返回”的时候被其他事件修改。C16 的主线会反复使用这个约束：只在需要等待一个明确条件时进入局部循环，并且必须有正常完成和超时退出。

最小结构如下：

```cpp
QEventLoop loop;
QTimer frame_timer;
QObject::connect(&frame_timer, &QTimer::timeout, &loop, [&] {
    if (frame == frames) {
        loop.quit();
        return;
    }
    renderPreviewFrame(frame++);
});
QTimer::singleShot(timeout_ms, &loop, [&] { loop.exit(1); });
frame_timer.start(0);
loop.exec();
```

`start(0)` 不是忙等。它把 timer event 排到事件队列后面，让同一轮事件分发有机会处理其他 queued event。这里的关键不是“0ms 更快”，而是“下一次由事件循环驱动”。如果直接同步调用三次 `renderPreviewFrame()`，检查也许能得到三个 frame，但你没有验证 Qt 事件来源、退出路径和重入边界。

`QCoreApplication` 是事件循环的宿主。无 GUI 的练习使用它；有窗口和 accessibility 的章节会换成 `QApplication`。一个进程只能有一个 application 对象，生命周期要覆盖所有 QObject 事件投递。MediaWorkbench 真正的主窗口会在后续章节加入，但事件循环模型已经足够支撑本章练习。

失败前提要分开看：

- 空任务应在进入 `exec()` 前拒绝，否则会创建一个永远等不到 frame 的循环。
- 超时是本地保护，不是业务成功。它证明教学示例有限结束。
- `exec()` 返回只说明循环退出，不说明任务成功；必须用显式 exit code 或状态区分正常完成、拒绝和超时。
- 运行结果来自当前进程的 Qt 事件分发；不代表真实播放器已经和系统多媒体后端连通。

练习 [L01_application_event](../exercises/L01_application_event/README.md) 要实现 `PreviewController::runPreview`。Student 只改 `student/solution.hpp`，Reference 和 good 都使用局部 `QEventLoop`，bad 则同步填完事件，检查器会拒绝它。下游 L04 会专门讨论嵌套 event loop 和重入；L05 使用 worker 线程后，结果仍要回到主线程事件循环；L07 的 session restore 也要等应用对象建立后才能安全恢复窗口状态。
