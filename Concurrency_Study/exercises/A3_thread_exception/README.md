# A3：异常从 worker 回到调用者

先读 [02 结果通道](../../chapters/02-result-channels.md)及 [03 的异常通道推导](../../chapters/03-threads-and-execution.md)。[main.cpp](main.cpp) 已安全捕获一个真实 worker 异常，作为修改起点；[solution.cpp](solution.cpp) 检查正常、业务失败和放弃路径。

## Part 与答案

### Part 1：手动 exception_ptr 通道

扩展 Starter 为成功/失败两个输入，worker catch(...) 写入 captured，主线程 join 后读。对应 `part1_manual(false/true)`。

答案：成功时 value=42 且无业务异常；失败时重抛 job_failure，消息为 manual failure。只有一个 worker 写槽，第一次读在 join 后，因此没有该槽上的数据竞争。多个 worker 不能同时无锁写同一槽。current_exception 在活动 handler 中取得当前异常；没有活动 handler 时为空。

### Part 2：将业务结果与错误交给同一 future

对应 `part2_promise(false/true)`。promise 移进 worker；正常 set_value(42)，业务 catch 中 set_exception；调用者 get 取得值或指定 job_failure，之后 future 无效。

外层 transport_error 只接结果通道操作本身的异常，主线程先 join 再读。Reference 不用一个宽泛 catch 吞掉检查失败，不把所有异常都视为预期业务错误。

### Part 3：放弃与原异常不是同一件事

对应 `part3_abandoned()`。销毁未满足 promise，再检查 get 抛 future_error 且码为 broken_promise。它说明 provider 放弃责任，不能代替传送实际的 job_failure。进程 terminate 不保证这套正常析构还能让主线程继续消费结果。

## 自测与验收

主线程 try/catch 为何接不住裸 worker 的顶层异常？因为它们不处于同一调用栈；必须先在 worker 内捕获，再通过通道在主线程重抛。这个错误情形只作为 Starter 注释，不进入测试。

join 后是否还能再显式 join？不能，第二次会出错；jthread 析构看到不再 joinable 才安全跳过。Reference 应输出手动/自动通道各两个分支、放弃分支和 `A3_reference OK`。异常的类型、消息及正常值都参与检查。

规范：[线程入口与构造](https://eel.is/c++draft/thread.thread.constr)、[共享状态](https://eel.is/c++draft/futures.state)。固定规范版本见正文 N5050 链接。

## 构建与运行

从 `Concurrency_Study/exercises` 执行（VS2026 生成器需要 CMake 4.2+）：

```powershell
cmake -S A3_thread_exception -B build/A3_thread_exception -G "Visual Studio 18 2026" -A x64
cmake --build build/A3_thread_exception --config Release
./build/A3_thread_exception/Release/A3_thread_exception.exe
ctest --test-dir build/A3_thread_exception -C Release --output-on-failure
```

统一构建注册的检查目标是 `A3_thread_exception_reference`，CTest 使用进程级超时。
