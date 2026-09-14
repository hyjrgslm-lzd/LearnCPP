# 04 分发、嵌套事件循环与重入

事件驱动程序最难的 bug 往往不是“事件没来”，而是“事件来得太早”。当 handler 还没返回时，你调用 `processEvents()`、进入局部 `QEventLoop`，或间接触发同步 signal，另一个事件可能进入同一个对象。这个对象如果正处在半更新状态，就会发生重入错误。

MediaWorkbench 的例子是审阅队列。用户点击“打开 A”，handler 正在清理旧预览；它中途进入一个局部循环等待确认，结果“打开 B”的事件先执行，修改了当前媒体。等 A 的 handler 恢复执行时，它继续使用旧假设，把 B 的状态覆盖掉。有限 event loop 本身不是错，错在没有定义重入边界。

最小防线是把“正在分发”作为状态：

```cpp
if (draining_) {
    retry_ = true;
    return;
}
draining_ = true;
// 只处理进入 drain 时已经存在的一批事件
draining_ = false;
```

为什么只处理当前批次？因为 handler 内新增的事件应该排在当前 handler 之后。否则内部 `drain()` 会让新事件插队，破坏“当前回调原子完成”的直觉。Qt 自己的事件队列也有类似的顺序边界：queued signal 是投递，不是任意时刻抢占 C++ 栈。

`clear()` 的语义也要准确。它应删除 pending 事件，但不能让正在执行的 handler 消失。同步取消一个已进入调用栈的函数没有通用安全办法；只能让 handler 自己在检查点观察状态。这个思想会在 L05 worker cancel 中再次出现：取消不能排队到已经阻塞的 worker slot，必须写共享状态并释放等待点。

重入不是多线程才有。单线程 GUI 也会重入，因为嵌套 event loop 允许新的事件在旧调用栈里执行。多线程增加的是数据竞争；重入增加的是状态机顺序错误。两者都要防，但证据不同：重入可以用单线程可复现队列检查，数据竞争需要线程和同步边界。

练习 [L04_reentrancy_dispatch](../exercises/L04_reentrancy_dispatch/README.md) 实现一个小型 dispatch queue。检查器让 handler 内部再次 `drain()`，要求 inner 事件等 outer 后半段完成后再运行。bad 版本直接 `while` 清空队列，会让 inner 插队。后续 L07 保存会话时，关闭和保存顺序也必须避免嵌套事件导致旧状态覆盖新状态。
