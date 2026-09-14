# 03 moc 元对象、信号槽与连接上下文

Qt 的信号槽不是普通 C++ 回调表。带 `Q_OBJECT` 的类会经过 moc 生成元对象信息，记录 signal、slot、属性和调用入口。`QObject::connect` 使用这些信息建立连接；发射 signal 时，Qt 根据连接类型、发送线程和接收者线程决定同步调用还是排队投递。

MediaWorkbench 的 `MediaSource::frameReady(QString,int)` 看起来只是传两个参数，但它同时携带三个契约：

1. 参数类型必须能被元对象系统处理。跨线程 queued connection 需要能复制参数。
2. receiver/context 决定连接生命周期。context 销毁后，Qt 自动断开，不调用悬空对象。
3. `Qt::AutoConnection` 的判定发生在发射时，而不是 connect 时。对象后续 `moveToThread()` 会改变跨线程判定。

常见错误是连接一个没有 context 的 lambda：

```cpp
connect(source, &MediaSource::frameReady, [sink](QString id, int frame) {
    sink->accept(id, frame); // sink 销毁后悬空
});
```

正确写法把 `sink` 作为 context：

```cpp
connect(source, &MediaSource::frameReady, sink,
        [sink](const QString& id, int frame) { sink->accept(id, frame); },
        Qt::AutoConnection);
```

同线程发射时，Auto 通常是 DirectConnection，slot 立即在当前调用栈执行。跨线程发射时，Auto 变成 QueuedConnection，slot 被投递到 receiver 所在线程的事件循环。这里说的是“发射线程”和“receiver 线程”，不是 sender 对象最初创建线程。检查跨线程行为时要真实移动对象或从 worker 线程发射，不能只读 connect 代码猜测。

参数也有边界。`QString`、`int` 这类 Qt 已知或可复制类型可以排队。自定义类型用于 queued connection 时，需要声明 metatype 并注册，否则运行时会无法排队。C16 后面的媒体帧不会直接把巨大图像对象塞进信号；通常传轻量 id、时间戳、共享缓冲句柄或不可变值。

练习 [L03_meta_connections](../exercises/L03_meta_connections/README.md) 要实现一个带 `Q_OBJECT` 的 `MediaSource` 和 `FrameSink`。检查器覆盖同线程参数、receiver 销毁后自动断开、`AutoConnection` 在 worker 发射时排到主线程。bad 版本使用无 context lambda，会在销毁和线程测试中暴露问题。L05 的 worker 结果回调、L06 model 的 `dataChanged`、L08 accessibility 控件事件都建立在本章连接语义上。
