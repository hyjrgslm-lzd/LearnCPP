# L05 ctypes

本题从 Python 直接调用 L05 专用 instrumented C18 插件 DLL。目标不是包装出漂亮 API，而是证明 Python 端声明的 ABI 与 C 头文件一致，并且 callback/userdata 活到插件销毁之后。instrumented DLL 的业务行为和 L04 good plugin 一致，另有只读计数导出供 checker 观察 native 调用。

Student 编辑文件：`student/solution.py`。骨架只保存库路径并安全失败，不会偷偷调用 Reference。

## Part 1：声明 C ABI

按 `include/c18/abi.h` 写出 ctypes 类型：

- `uint32_t` -> `ctypes.c_uint32`
- `size_t` -> `ctypes.c_size_t`
- `c18_context*` -> `ctypes.c_void_p`
- Windows `__cdecl` callback -> `ctypes.CFUNCTYPE`
- `c18_host_api` / `c18_api` -> `ctypes.Structure._fields_`

字段顺序必须和 C 结构体一致。`struct_size` 传 `ctypes.sizeof(...)`，让 C 端按真实结构前缀判断兼容性。

## Part 2：打开真实插件

用 `ctypes.CDLL(Path(library).resolve())` 加载 `$<TARGET_FILE:c18_l05_instrumented_plugin>`，再用 `c18_get_api(1, sizeof(Api), byref(api))` 取得函数表。`api.create` 成功后保存 `ctx`，后续所有调用都用这个 opaque handle。

## Part 3：处理 bytes 与 small buffer

`process(payload)` 为输出创建同长 `bytearray`，再调用 `process_into(payload, output)`。输入用 `from_buffer_copy`，输出用 `from_buffer`，因为输出归调用者并要被插件写入。

容量不足时，`process_into` 应返回 `(C18_STATUS_BUFFER_TOO_SMALL, required)`，并保持调用者原输出内容不变。checker 用哨兵 bytearray 证明这一点。

## Part 4：callback userdata 存活

不能把临时 `CFUNCTYPE(lambda...)` 直接塞进结构体。session 要保存：

- `_event`：C callback wrapper 的强引用。
- `_owner`：`ctypes.py_object(self)` 的强引用。
- `_owner_ptr`：传给 C 的 `void* userdata`。

C 回调进来后，把 userdata cast 回 `py_object*`，取出 session，再调用 Python callback。callback 抛异常时，trampoline 捕获并记录到 `callback_error()`，不能让异常穿过 C callback 边界。

checker 会创建 callback 后只留下 weakref，调用 `set_callback` 后删除自己的强引用并 `gc.collect()`。正确 session 必须仍让 weakref 活着；bad 只弱持有 callback，会在真实 native callback 后被拒绝。

## Part 5：关闭与重试

`close()` 调用 `request_stop(ctx)` 后调用 `destroy(ctx)`。如果 destroy 返回 `C18_STATUS_BUSY`，不要清掉 ctx/callback/userdata；下一次 close 要能重试。destroy 成功后再释放 callback、`ctypes.py_object`、`CFUNCTYPE` wrapper 和 ctx。重复 close 返回 OK；close 后 `process_into` 返回 `C18_STATUS_CLOSING`。

本题不使用 `_ctypes` 私有卸载函数，不证明 DLL 物理卸载。它只证明 context 和 callback owner 已经按 C18 协议结束。

## Reference / good / bad

- `reference/solution.py`：完整答案。
- `validation/good/solution.py`：独立好实现。
- `fixtures/instrumented_plugin.c`：真实 C ABI 插件，额外导出 observer 计数和一次 busy-destroy 注入。
- `validation/bad/solution.py`：真实 query/create/process 原生插件，但只 weakref 保存 callback target。
- `checks.py`：独立 observer 读取 native 调用计数，检查 bytes、callback weakref、callback 异常记录、small buffer、busy close 重试、close 后拒绝、重复 close。

## 构建与运行

Windows Release：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L05_ctypes -B C18_Interoperability_Plugins_Tooling/build/l05 -G "Visual Studio 18 2026" -A x64
cmake --build C18_Interoperability_Plugins_Tooling/build/l05 --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/l05 -C Release --output-on-failure
```

期望：`c18_l05_reference`、`c18_l05_good`、`c18_l05_bad_rejected` 通过。bad 通过的含义是 checker 稳定拒绝坏实现。

要看 Student 当前失败：

```powershell
cmake -S C18_Interoperability_Plugins_Tooling/exercises -B C18_Interoperability_Plugins_Tooling/build/python-student -DC18_BUILD_REFERENCE=OFF -DC18_TEST_STUDENTS=ON
cmake --build C18_Interoperability_Plugins_Tooling/build/python-student --config Release --target c18_l05_instrumented_plugin
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/python-student -C Release -R "c18_l05_student" --output-on-failure
```

期望：Student 失败，当前诊断是 `solution did not query c18_get_api`。这不是课程实现失败；它证明 starter 没预填答案。
