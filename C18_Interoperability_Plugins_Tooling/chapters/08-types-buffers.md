# 08. 类型与 Py_buffer：借用窗口必须关闭

Buffer protocol 让任意对象把内部存储暴露成 `Py_buffer`。这不是把内存所有权交给 C；这是 exporter 和 consumer 之间的一段借用窗口。

当前练习只用 `PyBUF_SIMPLE`，所以窗口里最关键的字段是：

```text
view.buf      指向连续字节
view.len      字节数
view.readonly 是否只读
view.obj      exporter 的强引用，release 前保持 exporter 存活
```

`view.obj` 是很多人漏掉的点。`PyObject_GetBuffer(obj, &view, flags)` 成功后，CPython 要求 `view.obj` 持有 exporter。consumer 调 `PyBuffer_Release(&view)` 时，这个引用才释放。也就是说，consumer 不能只把 `view.buf` 当普通裸指针用完就算结束。

L07 的 `ExportBox` 是最小 exporter。它内部只持有 exact `bytes` 对象和一个 `exports` 计数。保留 exact bytes 契约是为了少讲一层 GC：如果允许任意 bytes subclass，就要说明对象图和 GC 责任。本题先把 buffer 借用窗口讲清楚。类型对象的 `tp_as_buffer` 指向 `PyBufferProcs`：

```cpp
PyBufferProcs buffer_procs{getbuffer, releasebuffer};
ExportBoxType.tp_as_buffer = &buffer_procs;
```

`bf_getbuffer` 的责任是填充 view，并记录“现在有一个活跃借用”：

```cpp
if (PyBuffer_FillInfo(view, exporter, data, size, 1, flags) < 0) return -1;
++self->exports;
return 0;
```

`PyBuffer_FillInfo` 会设置 `view.obj` 等字段。因为 exporter 是只读 bytes，`readonly` 传 1。若 consumer 请求 `PyBUF_WRITABLE`，`PyBuffer_FillInfo` 必须失败，并且不能增加 `exports`。这里没有暴露 writable buffer，也没有 stride/format；这些高级字段不影响本章主线。

`bf_releasebuffer` 是对称退出路径：

```cpp
--reinterpret_cast<ExportBox*>(exporter)->exports;
```

Python 层的 `memoryview(box)` 会触发同样流程。checker 先创建 `view = memoryview(box)`，此时 `box.exports() == 1`；再 `view.release()`，计数回到 0。这证明 exporter 的 get/release 两端都接进 CPython 协议。

`ExportBox.__init__` 也要受这个窗口约束。Python 允许再次调用 `box.__init__(new_bytes)`；若还有活跃 view，不能替换内部 bytes，更不能把 `exports` 清零。正确行为是抛 `BufferError`，让旧 view 继续看旧 bytes。等所有 view release 后，再取得新 bytes 引用、交换字段、释放旧引用。

consumer 的最小正确调用是：

```cpp
Py_buffer view{};
if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) < 0) return nullptr;
// 只在这里读取 view.buf/view.len
PyBuffer_Release(&view);
```

真实实现还要处理失败 cleanup。若 `PyObject_GetBuffer` 失败，不能 release 未初始化 view。若中间分配输出失败，必须先 `PyBuffer_Release(&view)` 再返回 `nullptr`。本练习直接用 `PyBytes_FromStringAndSize(nullptr, view.len)` 分配返回对象，然后填字节。这样失败就是 Python exception + `nullptr`，不会让 C++ `std::bad_alloc` 穿过 Python C ABI，也不会漏掉已经取得的 `Py_buffer`。

bad 版本故意把字节转换做对，但少了 `PyBuffer_Release(&view)`。这种错误比“输出错”更像真实 bug：调用结果可能正确，exporter 却一直认为有人借着内部存储。对可变 exporter 来说，这会阻止 resize；对持有外部资源的 exporter 来说，这会拖延释放。

修复不是在 Python 层 `del view`，而是在 C consumer 每个成功 `GetBuffer` 路径上配对 `PyBuffer_Release`。checker 还会用 `ExportBox.__new__(ExportBox)` 制造未初始化对象，要求 `getbuffer` 安全抛错；用非连续 memoryview 调 `transform_view`，要求 consumer 失败时不假装读连续内存。bad 版本只保留一个真实错误：转换结果正确，但忘记 `PyBuffer_Release`，所以 `exports()` 留在 1。其它创建、reinit 和错误路径都应正确，避免把多个问题混成一个诊断。
