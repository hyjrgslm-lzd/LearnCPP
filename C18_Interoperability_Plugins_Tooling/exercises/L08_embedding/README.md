# L08 embedding

Embedding 的顺序是资源契约，不是风格问题：

1. 用 `PyConfig` 或明确初始化入口建立解释器。
2. C++ 线程进入 Python 前必须持有对应 GIL。
3. 停止 worker 后先 join，让所有 `PyObject*`、borrowed 引用升级得到的 strong reference、callback 状态都释放。
4. 最后才 `Py_FinalizeEx`。

本题的 Reference 使用一个有界 worker，不是通用任务系统：

- 构造函数初始化 CPython 3.10.11，随后主线程 `PyEval_SaveThread()` 放开 GIL。
- `transform()` 把请求交给 worker；worker 用 `PyGILState_Ensure()` 进入 Python，执行 bytes 转换，拷贝结果，再释放所有本次创建的 `PyObject*` strong references。
- `stop()` 只发布停止请求。
- `join()` 真 join worker，再恢复主线程 `PyThreadState*`，最后 `Py_FinalizeEx()`。

checker 读取事件 trace：worker 线程 id、GIL 获取点、Python 结果 bytes、refs 释放、stop 被 worker 读到、join、finalize。`released_before_shutdown()` 只是兼容观察接口，不能用自报布尔值冒充通过。

bad 会在 worker join 前尝试 finalize；实现里的 gate 拒绝这个危险动作并记录，checker 用该记录拒绝 bad。Student 空实现应能安全运行并真实失败。
