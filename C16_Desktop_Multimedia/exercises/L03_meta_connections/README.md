# L03 meta connections

对应正文：[03-metaobject-connections](../../chapters/03-metaobject-connections.md)。

学生只编辑 `student/solution.hpp`。完成 `MediaSource`、`FrameSink` 和 `connectFrames`：

1. `publish` 真实发射 moc signal。
2. signal 参数携带媒体 id 和帧号。
3. 连接必须带 receiver/context，receiver 销毁后自动断开。
4. 使用 `Qt::AutoConnection`，跨线程发射时 slot 在 receiver 线程执行。

`bad` 使用无 context lambda，检查器用销毁和跨线程发射拒绝它。
