# L07 Python buffers

本题实现一个最小 exporter 和一个 consumer。目标是看清 `Py_buffer` 的借用窗口：`GetBuffer` 成功后 exporter 必须活着，consumer 用完必须 `Release`。

Student 编辑文件：`student/solution.cpp`。骨架只导出 `transform_view` 并抛 `NotImplementedError`，还没有 `ExportBox` 类型。

## Part 1：实现 exporter

定义 `ExportBox`：

- 内部保存一个 `PyObject* payload`，本题契约要求 exact `bytes`。
- 内部保存 `int exports`，记录活跃 view 数。
- `tp_dealloc` 释放 payload。
- `exports()` 返回当前计数，供 checker 观察协议状态。

类型对象要设置 `tp_as_buffer = &buffer_procs`，否则 `memoryview(box)` 不会走 buffer protocol。

## Part 2：bf_getbuffer

`bf_getbuffer(exporter, view, flags)` 先拒绝未初始化对象，再从内部 bytes 取出 `char* + Py_ssize_t`，再调用：

```cpp
PyBuffer_FillInfo(view, exporter, data, size, 1, flags);
```

这里最后一个 owner 关系由 CPython 维护：成功后 `view.obj` 持有 exporter。只有 `PyBuffer_FillInfo` 成功后才能 `++exports`，表示有一个活跃借用。因为 exporter 是只读 bytes，`PyBUF_WRITABLE` 请求必须失败，且不能改变 `exports`。

## Part 3：bf_releasebuffer

`bf_releasebuffer(exporter, view)` 做对称释放：`--exports`。Python 的 `memoryview.release()` 会触发它。checker 先单独验证 `memoryview(box)` 让计数从 0 到 1，再 release 回 0。

`__init__` 可以被 Python 再次调用。若当前 `exports != 0`，必须抛 `BufferError`，不能替换 payload，也不能把计数清零。所有活跃 view release 之后，才允许用新的 bytes 重新初始化。

## Part 4：consumer transform_view

`transform_view(obj)` 用 `PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE)` 取得 view，只在这个窗口内读 `view.buf/view.len`，用 `PyBytes_FromStringAndSize(nullptr, view.len)` 先创建返回 bytes，再逐字节写入。返回前调用 `PyBuffer_Release(&view)`。

如果中间发生失败，已经成功取得的 view 仍要 release。不要在 `GetBuffer` 之后先分配 `std::string` 再 release；C++ `bad_alloc` 不能穿过 Python C API 返回给解释器。要么只用 CPython 分配函数并按 `nullptr` 返回，要么用真正 RAII 并把 C++ 异常转换成 Python exception。

## Reference / good / bad

- `reference/solution.cpp`：完整答案。
- `validation/good/solution.cpp`：独立好实现；先 snapshot 成 bytes，release 后调用 `bytes.upper()`。
- `validation/bad/solution.cpp`：转换结果正确，但忘记 `PyBuffer_Release`。
- `checks.py`：验证 `memoryview` get/release、active view 下 reinit 拒绝、release 后 reinit 成功、未初始化对象安全拒绝、readonly writable 请求、非连续 view 拒绝、`transform_view` 输出和最终 `exports()==0`。

## 构建与运行

Windows Release：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L07_python_buffers -B C18_Interoperability_Plugins_Tooling/build/l07 -G "Visual Studio 18 2026" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release -DC18_ENABLE_PYTHON=ON
cmake --build C18_Interoperability_Plugins_Tooling/build/l07 --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/l07 -C Release --output-on-failure
```

期望：reference/good 通过；bad_rejected 通过，表示 checker 拒绝漏 release。

Student 验证：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises -B C18_Interoperability_Plugins_Tooling/build/python-student -DC18_BUILD_REFERENCE=OFF -DC18_ENABLE_PYTHON=ON -DC18_TEST_STUDENTS=ON
cmake --build C18_Interoperability_Plugins_Tooling/build/python-student --config Release --target c18_l07_student
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/python-student -C Release -R "c18_l07_student" --output-on-failure
```

期望：Student 当前失败，因为 exporter 和 release 协议尚未实现。
