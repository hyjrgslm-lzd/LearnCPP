# L10 pybind11

本题固定 pybind11 3.0.4。默认 `C18_ENABLE_PYBIND11=OFF` 时只在 configure 输出 disabled，不注册主体 SKIP。显式 `-DC18_ENABLE_PYBIND11=ON` 后必须找到已有的 pybind11 3.0.4 EXACT；缺失就是配置失败，不下载、不伪造通过。本机当前主体状态是 UNVERIFIED。

必须覆盖的点：

- `return_value_policy::reference_internal`：`Owner::view()` 返回 C++ `View&` 成员，`View.__bytes__()` 才生成 Python bytes；checker 删除 owner 后先确认 owner 仍活，再读取 view。
- `keep_alive`：`Registry.remember(token)` 不保存 token；checker 删除 token 后用析构计数证明 `py::keep_alive<1, 2>()` 生效。
- holder：`Owner` 使用 `std::shared_ptr` holder；`make_owner_alias()` 返回实际持有 `shared_ptr<Owner>` 的 `OwnerAlias`，checker 删除原 owner 后确认 alias 仍能读 owner 状态，再删除 alias 观察析构。
- trampoline/GIL：`Transformer` 的 Python override 必须由 C++ worker 线程实际调用；worker 获取 GIL，主线程 join 前释放 GIL。
- exception：`raise_cpp()` 的 `std::runtime_error` 应在 Python 侧变成 `RuntimeError`，消息保留。
- embedding/GIL：`c18_l10_embed` 用 `py::scoped_interpreter`，所有 `py::object` 在 guard 析构前离开作用域，不混裸 `Py_Initialize`。

`reference`、`good`、`bad`、`student` 源码保留完整练习入口。启用 pybind11 后 student 总是构建；`C18_TEST_STUDENTS` 只控制是否注册 student 检查。`C18_BUILD_REFERENCE=OFF -DC18_TEST_STUDENTS=ON` 时只测试 student 模块，不引入 Reference。bad 的生命周期错误在观察到 token 过早析构后停止，不访问已死对象。
