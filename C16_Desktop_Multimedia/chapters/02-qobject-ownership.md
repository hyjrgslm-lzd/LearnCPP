# 02 QObject 所有权、树与延迟销毁

MediaWorkbench 的审阅器会有主窗口、控制条、媒体条目、后台任务和临时对话框。Qt 不要求所有对象都用 `std::unique_ptr` 表达所有权，而是大量使用 QObject parent tree：父对象析构时删除 children。这个模型简化 UI 树和组件生命周期，但前提是树上的对象必须适合由 parent 删除。

最安全的基础规则是：由 parent 管的 QObject 放在 heap 上创建，由 parent 负责释放；外部对象、栈对象、由其他智能指针管理的对象，不随便 `setParent()` 接管。下面这个结构是合法的：

```cpp
auto window = std::make_unique<QObject>();
auto* clip = new QObject(window.get());
clip->setObjectName("clip");
window.reset(); // clip 被 QObject tree 删除
```

危险写法是把栈对象塞进 parent tree：

```cpp
QObject clip;
clip.setParent(window); // window 先析构会 delete 一个栈对象
```

这不是“Qt 不喜欢栈对象”，而是所有权契约冲突。栈对象由作用域释放；QObject parent tree 会在父对象析构时 `delete` child。两个拥有者同时认为自己负责释放同一块对象，结果就是未定义行为。C++ 所有权和 Qt parent tree 可以一起用，但边界要清楚：`unique_ptr` 通常拥有根对象，根以下交给 QObject tree。

`deleteLater()` 处理另一个常见问题：对象可能正在处理事件或有 queued signal 即将到达。同步 `delete` 会让当前事件之后的投递拿到悬空指针。`deleteLater()` 投递一个 deferred delete event，对象会在事件循环回到安全点时析构。观察对象是否已经失效，用 `QPointer<T>`，不要缓存裸指针再猜生命周期：

```cpp
QPointer<QObject> watched = object;
object->deleteLater();
QCoreApplication::processEvents();
if (!watched) {
    // 对象已经析构，不能再访问原指针
}
```

隐式共享类型如 `QString`、`QByteArray`、`QImage` 不是 QObject。它们按值复制，修改时 detach。不要把“Qt 对象”混成一种模型：QObject 关注身份、线程归属、parent 和事件；隐式共享值关注数据缓冲和写时复制。MediaWorkbench 的 session JSON 用 `QByteArray` 保存窗口几何，那是值；播放按钮和 worker 是 QObject，那是身份。

练习 [L02_qobject_lifetime](../exercises/L02_qobject_lifetime/README.md) 覆盖三件事：parent 删除 child、拒绝接管外部对象、`deleteLater()` 后 `QPointer` 清空。Reference/good 都不把栈对象塞进树；bad 漏 parent、接管外部对象并同步 delete。后续 L03 的 context disconnect、L05 的 receiver 提前销毁、L08 的 widget 层级都依赖本章的对象身份和销毁规则。
