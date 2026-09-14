# Python Source Reading

本页固定本课 Python 线的源码阅读对象和证据边界。不要用滚动 `main` 或未来版本文档替代这里的 3.8/3.10 结论。

## 固定版本

- CPython 3.10.11：L06/L07 Full C API、L08 embedding/GIL、P1 Python backend 的当前验证版本。
- CPython 3.8.10：L09 Limited API floor，使用 `Py_LIMITED_API=0x03080000` 和 Windows `python3.lib`。
- pybind11 3.0.4：L10 固定目标版本；当前环境未安装，相关材料为 UNVERIFIED。

官方文档入口：

- Python 3.10 C API intro：`https://docs.python.org/3.10/c-api/intro.html`
- Python 3.10 exceptions/error indicator：`https://docs.python.org/3.10/c-api/exceptions.html`
- Python 3.10 initialization/thread state：`https://docs.python.org/3.10/c-api/init.html`
- Python 3.10 initialization config：`https://docs.python.org/3.10/c-api/init_config.html`
- Python 3.10 buffer protocol：`https://docs.python.org/3.10/c-api/buffer.html`
- Python 3.8 Stable ABI：`https://docs.python.org/3.8/c-api/stable.html`
- Python 3.8 Windows extending/build notes：`https://docs.python.org/3.8/extending/windows.html`
- pybind11 functions/policies：`https://pybind11.readthedocs.io/en/stable/advanced/functions.html`
- pybind11 embedding：`https://pybind11.readthedocs.io/en/stable/advanced/embedding.html`

## CPython 源码入口

引用计数：

- 声明入口：`Include/object.h`
- 关键宏/函数：`Py_INCREF`、`Py_DECREF`、`Py_XDECREF`、`Py_NewRef`
- 实现入口：`Objects/object.c`
- 对应练习：L06 `make_owner`、`borrowed_as_new`、`tuple_steals`

错误状态：

- 声明入口：`Include/pyerrors.h`
- 实现入口：`Python/errors.c`
- 关键动作：`PyErr_SetString` 设置当前线程异常，`PyErr_Clear` 清空，C 函数返回 `nullptr` 表示失败
- 对应练习：L06 `clear_error_then_return`

扩展模块初始化：

- 导入路径入口：`Python/import.c`
- 模块对象：`Objects/moduleobject.c`
- 关键形态：扩展模块导出 `PyInit_<module>`，返回 new module object 或 `nullptr`
- 对应练习：L06/L07/L09 的 `.pyd` 动态导入

类型对象：

- 声明入口：`Include/object.h`、`Include/cpython/object.h`
- 关键动作：填 `PyTypeObject` 字段，调用 `PyType_Ready`，加入模块前管理类型对象引用
- 对应练习：L06 custom owner type，L07 `ExportBox`
- 边界：这属于 Full C API 训练，不拿来证明 Limited API

Buffer protocol：

- 声明入口：`Include/object.h` 中 `PyBufferProcs` / `Py_buffer`
- memoryview 行为：`Objects/memoryobject.c`
- 关键动作：exporter 实现 `bf_getbuffer/bf_releasebuffer`，consumer 调 `PyObject_GetBuffer/PyBuffer_Release`
- 对应练习：L07 `ExportBox` 和 `transform_view`

Initialization / embedding：

- 配置入口：`Python/initconfig.c`
- 生命周期入口：`Python/pylifecycle.c`
- GIL/thread state：`Python/pystate.c`，3.10 源码树中的 GIL 实现相关文件
- 对应练习：L08 和 P1 Python backend

Stable ABI / Limited API：

- 文档入口固定 Python 3.8 `c-api/stable.html`
- 头文件边界：使用 Limited API 时避免依赖 `Include/cpython/*` 的内部结构字段
- Windows 二进制边界：abi3 目标应通过 `python3.lib/python3.dll`，不直接依赖 `python38.dll/python310.dll`
- 对应练习：L09，同一 `.pyd` 在 Python 3.8.10 和 3.10.11 导入

## 阅读顺序

1. 从练习调用点出发：例如 `PyList_GetItem`、`PyTuple_SetItem`、`PyObject_GetBuffer`。
2. 在官方文档确认返回引用类型、错误返回约定和版本边界。
3. 回到 CPython 固定版本源码，找状态变化：引用计数、error indicator、`view.obj`、exports、module init。
4. 回到 checker，说明哪条证据证明该状态被正确收束。

本页不复制 CPython 大段源码。课程要训练的是入口定位和状态推导，不是把源码粘进教材。
