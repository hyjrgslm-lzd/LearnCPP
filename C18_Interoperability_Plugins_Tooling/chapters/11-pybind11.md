# 11. pybind11：省样板，不省 owner 证明

pybind11 减少 CPython C API 样板，但不取消跨边界责任。`return_value_policy`、holder、`keep_alive`、trampoline、异常翻译和 GIL 都是在表达同一件事：谁保证对象活着，谁负责把错误放到正确语言里。

本课固定 pybind11 3.0.4。当前环境没有安装，且任务禁止下载；因此 L10 默认是 disabled，主体结果记录为 `UNVERIFIED`。disabled 不注册主体 SKIP 测试；只有显式 `-DC18_ENABLE_PYBIND11=ON` 后才进入 `find_package(pybind11 3.0.4 EXACT REQUIRED CONFIG)`。若依赖缺失，配置阶段失败。这比“跑一个 skip 测试”更清楚：源码存在，但本机没有证明 pybind11 路线可编译可运行。

第一个核心点是返回策略。C++ owner 返回一个依赖内部状态的 view 时，Python view 不能比 owner 活得更久。但不能把“已经构造好的 Python 对象”当成 `reference_internal` 证明：pybind11 3.0.4 的 `pyobject_caster::cast(const handle&, return_value_policy, handle)` 只给 handle 增引用，忽略 policy 和 parent。因此 L10 不用 `py::memoryview::from_memory()` 直接返回；它让 `Owner::view()` 返回 C++ 成员 `View&`，再把 `View` 绑定成 Python 类型，`View.__bytes__()` 负责生成 bytes。checker 删除 owner 后先检查 owner 析构计数仍为 0，再读取 `bytes(view)`，最后删除 view 并确认 owner 析构。

第二个点是 holder。跨语言共享对象不应返回裸指针假装长期有效；`std::shared_ptr` holder 至少让 C++/Python 双方看到同一控制块。L10 不用临时 `use_count()` 证明，因为 caster 过程可能复用 Python wrapper 或产生短暂计数。它暴露 `make_owner_alias(owner)`，返回实际持有 `shared_ptr<Owner>` 的 `OwnerAlias`。checker 删除原 owner 后确认 owner 没有析构、alias 仍能读取 owner 状态；删除 alias 后再确认 owner 析构。这个证明只依赖长期存活事实。

第三个点是 `keep_alive`。注册 callback、把 child 放进 parent、把 iterator 交给 Python 时，调用表达式要说明依赖关系。L10 的 `Registry.remember(token)` 故意不在 C++ 对象里保存 token，只记录 token 名字；Reference/Good 靠 `py::keep_alive<1, 2>()` 让 Python 调用关系保活 token。checker 删除 Python 端 token 后先看析构计数必须仍为 0，再删除 registry 后看析构计数变为 1。bad 少这个 policy，token 在 registry 还活着时就析构；checker 在观察到析构后停止，不读取已死对象。

第四个点是 trampoline 和 GIL。C++ 虚函数回调到 Python override 时，线程必须持有 GIL；Python 异常要翻译回 C++ 错误通道，再在 C ABI 边界前收束，不能直接穿过插件 ABI。L10 的 `Transformer` 有 C++ base 实现和 `PyTransformer` trampoline，`call_transformer_from_worker()` 启动一个 C++ worker，worker 用 `py::gil_scoped_acquire` 后调用虚函数。主线程在 join 时用 `py::gil_scoped_release` 放开 GIL，避免和 worker 互等。checker 的 Python subclass 返回 `override:AZ`，并读取 trace 中的 `worker_gil` 与 `override_returned:override:AZ`，证明结果来自 Python override，不是 base fallback。

第五个点是异常翻译。L10 的 `raise_cpp()` 抛 `std::runtime_error("mapped to Python exception")`；checker 在 Python 侧期待 `RuntimeError`，并检查消息仍在。这里验证的是 pybind11 的语言边界转换，不是 C ABI；如果这一路外面还有 C ABI wrapper，wrapper 仍必须 catch，不能让 C++ 异常穿 ABI。

pybind embedding 也要和裸 CPython embedding 分开。L10 单独提供 `c18_l10_embed` 程序：只创建 `py::scoped_interpreter`，在内层作用域创建 `py::dict`、`py::bytes` 和执行结果，让这些 Python 对象在 guard 析构前先离开作用域。它不调用裸 `Py_Initialize` 或 `Py_FinalizeEx`，避免两个初始化模型互相踩状态。

本章不把 pybind11 说成 Limited API 或 abi3。它是高层绑定库，方便表达 policy；是否能稳定跨 Python 小版本，需要按其实际构建选项和官方支持单独验证。
