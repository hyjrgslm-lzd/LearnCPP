# 09. Embedding 与 GIL：关闭顺序是契约

Embedding 把方向反过来：C++ 进程创建 Python 解释器，再把 Python 当作一个运行时组件调用。此时 C++ 拥有进程、线程、退出顺序和错误传播；Python 不会替 host 收尾。

本课 P1 后端要求 `c18::python_transform(std::span<const uint8_t>)`。它是私有 C++ 接口，可以抛 `std::runtime_error`，但 P1 的 C ABI 外壳不能让异常穿出去。实现必须真的执行 Python：把 input 构造成 `bytes`，在解释器中定义并调用 `transform(payload)`，再把返回 bytes 拷贝回 `std::vector<uint8_t>`。

初始化使用 `PyConfig`。`PyConfig_InitIsolatedConfig` 减少环境和 site 包对教学实验的干扰，`Py_InitializeFromConfig` 后要 `PyConfig_Clear`。失败时用 `PyStatus_Exception` 判断，不要继续调用 Python C API。

GIL 规则随线程进入。单线程 host 在初始化后默认持有可调用 Python 的线程状态；一旦把 Python 调用交给 worker，主线程不能继续捏着 GIL 等它。L08 的 Reference 基线按真实顺序做四件事：

1. 主线程用 `PyConfig_InitIsolatedConfig` 和 `Py_InitializeFromConfig` 初始化 3.10.11 解释器。
2. 主线程调用 `PyEval_SaveThread()` 释放 GIL，并保存主线程的 `PyThreadState*`。
3. worker 线程等待请求；每次真正进入 Python 前调用 `PyGILState_Ensure()`，把 payload 构造成 `bytes`，执行 Python bytes 转换，再 `DECREF` 本次创建的 strong references。
4. `stop()` 只发布退出请求；`join()` 真正 join worker，然后主线程用 `PyEval_RestoreThread(saved_state)` 恢复主线程状态，最后调用 `Py_FinalizeEx()`。

这个顺序的重点是“等待时不要持有 GIL，finalize 前必须没有 worker 仍可能碰 Python”。如果主线程初始化后直接等待 worker，而 worker 又需要 `PyGILState_Ensure()`，两个线程会互相等。若主线程在 worker join 前 finalize，worker 释放引用或退出 Python 调用时会碰已经拆掉的解释器。

关闭顺序不能反。若 worker 还持有 `PyObject*` 或还可能回调 Python，先 finalize 会把解释器状态拆掉，随后释放引用就进入未定义行为区域。L08 bad 不执行危险 finalize；它在 worker join 前提出 finalize 请求，runtime gate 看到 worker 仍 joinable 后拒绝并记录 `early_finalize_rejected`。checker 把这个记录视为真实错误动作：它证明实现走到了错误关闭分支，同时没有为了教学反例制造真实 UAF。

L08 checker 不再相信 `released_before_shutdown()` 这种自报布尔值。它读取实现记录的事件序列，检查这些事实：

- `worker_started`、`worker_gil_acquired` 和 `python_result` 的线程 id 必须不同于 main 线程。
- `python_result` 的 bytes 必须等于真实 Python 执行结果 `AZA\0\xffM`。
- `py_refs_released` 必须早于 `worker_joined`，`worker_joined` 必须早于 `finalized`。
- `stop_observed` 必须早于 `worker_joined`，说明 worker 读到了 stop 请求，而不是 main 线程单方面改一个标记。
- 不允许出现 `early_finalize_rejected`，除非正在验证 bad 被拒绝。

本章不把 CPython 嵌入写成常驻通用 runtime。L08 的 worker 只有一个请求槽、一个 condition variable 和一个停止状态；这足够证明 shutdown 契约，不扩展成任务系统。P1 每进程只调用一个后端，后端用一个互斥保护初始化/调用/finalize。吞吐不是目标；性能扩展归边界成本章节和 root 负责。这里要证明的是：真实执行 Python、所有 Py refs 在解释器关闭前释放、错误留在 C++ 私有边界内。

F01 已有独立子解释器观察和 free-threaded 扩展入口。它们说明未来运行时形态会变化，但不改变本章对 3.10 Full embedding 的关闭顺序要求。
