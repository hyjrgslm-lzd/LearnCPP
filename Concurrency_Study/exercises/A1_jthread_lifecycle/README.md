# A1：线程关联、join 与输入所有权

先读 [03 线程与执行方式](../../chapters/03-threads-and-execution.md)第 1—3 节，并完成 [P1](../P1_object_lifetime/README.md)。[main.cpp](main.cpp) 保留安全的 thread/jthread 基线；[solution.cpp](solution.cpp) 覆盖四个必做 Part。

## Part 与答案

### Part 1：显式 join 是一次关联的收尾

扩展 Starter，检查 worker 写入的 42 与 joinable 状态，再捕获第二次显式 join 的 system_error。对应 `part1_thread_join()`。线程完成同步于成功 join 返回，因此主线程在 join 后读取结果与异常槽安全。第二次显式 join 是错误；“jthread 已 join 后析构安全”来自析构不再调用 join，不能混为一谈。

### Part 2：作用域退出与移动

对应 `part2_scope_and_move()`。把 original 移到 owner，检查 original 不再 joinable，owner 仍 joinable。作用域结束后结果为 7。关联状态不等于线程函数仍在运行：函数早已返回的线程也可能尚未 join。

### Part 3：先请求停止，再等待结束

对应 `part3_automatic_stop()`。worker 接收 stop_token 并查询停止请求；主线程直接离开作用域，不先睡眠。保存 token 和结果标记，在作用域结束后检查两者。worker 可能一次循环也没执行，仍是正确结果：只要求观察请求后返回，不要求某个迭代次数。

yield 是调度提示，停止依据是 token，同步依据是 join。若改变实验让 worker 永远不结束，自动 join 也会一直等待。完整取消与唤醒协议继续 [A2](../A2_stop_token_cancellation/README.md)。

### Part 4：参数副本、引用、独占捕获

对应 `part4_arguments()`。按值传 10 得到副本 11，原值不变；std::ref 使 worker 修改原值，主线程 join 后才读；移动 unique_ptr 到闭包后原指针为空，结果为 42。借用不延寿，移动不意味着所有类型的源对象都为空。

## 验收与失败边界

Reference 使用 cs::check；会抛异常的 worker 逻辑捕获到专属 exception_ptr，主线程 join 后重抛。只做无抛出的标量操作与 token 查询的入口无需另造异常机制。输出四个 Part 与 `A1_reference OK`。

未 join 的 std::thread 析构和线程顶层异常只作为注释阅读；不要删掉 Starter 的清理调用来运行默认测试。本题没有 sleep，也不以日志毫秒数证明同步。

规范：[thread 成员](https://eel.is/c++draft/thread.thread.member)、[thread 构造](https://eel.is/c++draft/thread.thread.constr)、[jthread 构造析构](https://eel.is/c++draft/thread.jthread.cons)。版本基准见正文 N5050 说明。

## 构建与运行

从 `Concurrency_Study/exercises` 执行（VS2026 生成器需要 CMake 4.2+）：

```powershell
cmake -S A1_jthread_lifecycle -B build/A1_jthread_lifecycle -G "Visual Studio 18 2026" -A x64
cmake --build build/A1_jthread_lifecycle --config Release
./build/A1_jthread_lifecycle/Release/A1_jthread_lifecycle.exe
ctest --test-dir build/A1_jthread_lifecycle -C Release --output-on-failure
```

统一构建注册的检查目标是 `A1_jthread_lifecycle_reference`，CTest 使用进程级超时。
