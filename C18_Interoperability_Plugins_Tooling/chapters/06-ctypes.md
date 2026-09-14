# 06. ctypes：动态 ABI 不是自动安全

`ctypes` 让 Python 在运行时打开 DLL 并调用 C 函数。它只帮你发出调用，不会替你证明 ABI 正确。C 编译器原本会检查的事情，在 Python 端都变成你手写的声明：整数宽度、`size_t`、指针、结构体字段顺序、调用约定、callback 函数指针和 `void* userdata` 的存活。

本章仍沿用 C18 插件契约。插件只导出一个入口：

```c
c18_status c18_get_api(uint32_t requested_version,
                       uint32_t host_struct_size,
                       c18_api* out_api);
```

调用者拿到的不是一个“Python 对象”，而是一张 C 函数表。当前真实接口里，`c18_api` 前缀依次是 `version`、`struct_size`、`create`、`process`、`request_stop`、`destroy`。`create` 接收 `c18_host_api`，里面有 `userdata` 和 `on_event` callback。`process` 接收 `uint8_t* + size_t` 输入、host-owned 输出缓冲、输出容量和 `size_t* written`。

这个接口的所有权图很小：

```text
Python CtypesSession
  owns CDLL loader handle
  owns Api function table wrappers
  owns ctx returned by api.create
  owns CFUNCTYPE callback wrapper
  owns ctypes.py_object(self) used by void* userdata
  borrows input bytes only for one process call
  owns output bytearray passed to plugin

C plugin
  borrows host_api during create
  stores copied callback pointer + userdata value
  borrows input/output buffers only during process
  never frees host memory
```

最小正确调用先从类型声明开始。`uint32_t` 对应 `ctypes.c_uint32`，`size_t` 对应 `ctypes.c_size_t`，opaque `c18_context*` 用 `ctypes.c_void_p`。Windows 上 C18 ABI 使用 `__cdecl`，因此普通 `ctypes.CFUNCTYPE` 是匹配的；如果接口改成 `stdcall`，这里就必须改成 `WINFUNCTYPE`。结构体字段不能按“看起来一样”重排；`ctypes.sizeof(HostApi)` 和 C 端 `struct_size` 协商依赖真实布局。

小型基线如下。先声明 callback 类型，再声明结构体，再把函数表里的指针字段声明成 `CFUNCTYPE`。这样 `api.process(...)` 调用时，ctypes 才知道每个参数如何压栈或进寄存器。

```python
EVENT = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_char_p)

class Host(ctypes.Structure):
    _fields_ = [
        ("version", ctypes.c_uint32),
        ("struct_size", ctypes.c_uint32),
        ("userdata", ctypes.c_void_p),
        ("on_event", EVENT),
    ]
```

边界错误最容易出现在 callback。下面这种代码能跑过一次纯 `process`，但 callback 入口是临时 wrapper，离开构造函数后可能被 GC：

```python
host = Host(1, ctypes.sizeof(Host), None, EVENT(lambda _, name: print(name)))
```

修复不是“祈祷 GC 慢一点”，而是让 session 强持有 wrapper 和 userdata holder：

```python
self._owner = ctypes.py_object(self)
self._owner_ptr = ctypes.cast(ctypes.pointer(self._owner), ctypes.c_void_p)
self._event = EVENT(self._on_event)
host = Host(1, ctypes.sizeof(Host), self._owner_ptr, self._event)
```

这解释了 L05 的检查设计。checker 传入 `b"azA\0\xff!m"`，期望输出 `b"AZA\0\xff!M"`。`NUL` 和 `0xff` 证明输入不是 C 字符串；`!` 触发插件回调 `process:bang`，证明 C 函数指针和 userdata 仍然活着。

为了避免“Python 自己算出正确值”混过检查，L05 不再直接使用生产 P1 插件，而是使用一个本题专属 instrumented DLL。它的业务行为和 L04 good plugin 一致，但额外导出只读 observer：`c18_l05_query_counts`、`c18_l05_reset_counts` 和一次性 `c18_l05_set_destroy_busy_once`。checker 自己用 ctypes 读取 native 计数，确认被测 solution 真的调用了 `c18_get_api/create/process/request_stop/destroy`。

第二个边界是输出容量。`process_into(payload, small_buffer)` 传入短 bytearray，正确结果是 `C18_STATUS_BUFFER_TOO_SMALL`，`written == len(payload)`，并且原 bytearray 内容不变。插件不能写一半再告诉你容量不足；否则调用者无法安全重试。

Python callback 抛异常时，不能让异常穿过 C callback 边界。L05 的 trampoline 捕获 Python 异常，把文本记录在 session 里，然后让原生 `process` 正常返回。这个保证只覆盖 Python callback 错误：原始 C ABI 仍只用 `c18_status` 表达插件调用成败。

退出路径也必须写清楚。`close()` 先 `request_stop(ctx)`，再 `destroy(ctx)`。如果 `destroy` 返回 `C18_STATUS_BUSY`，session 必须保留 `ctx`、callback wrapper 和 userdata owner，允许下一次 close 重试；只有 destroy 成功，才清掉 `ctx`、callback、`py_object` 和 `CFUNCTYPE` 引用。L05 的 busy-destroy 注入夹具证明这条失败路径。重复 close 应返回 OK；close 后再 process 必须拒绝。

本题不调用 `_ctypes` 私有卸载函数，也不声称 DLL 在进程里被物理卸载。`ctypes.CDLL` 的生命周期属于测试进程；L05 只证明 plugin context 和 callback/userdata owner 被正确结束。

L05 bad 是真实代表性错误：它确实 query/create/process 原生插件，也能完成 bytes 转换；唯一错误是 `set_callback` 只保存 weak reference。checker 在 native callback 路径跑完后发现 callback target 已经被 GC，稳定拒绝 `"callback target was not kept alive"`。这比旧的“纯 Python 模拟结果”更接近实际 FFI bug。
