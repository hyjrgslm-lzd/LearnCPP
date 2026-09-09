# D3：调用与结果打包，交给明确的执行者

先读 [03 第 9—10 节](../../chapters/03-threads-and-execution.md)。[main.cpp](main.cpp) 在当前线程调用一个拥有 unique_ptr 的 task；[solution.cpp](solution.cpp) 完整检查五个 Part。此题不实现线程池类。

## Part 与答案

### Part 1：打包不执行，调用者选择执行位置

对应 `part1_direct_and_thread()`。构造 packaged_task 并 get_future 后状态未就绪；在主线程调用 task，结果 ID 等于主线程。另一个新 task 移入 jthread 后调用，结果 ID 不同，源 task 无效。返回值/业务异常由包装器进入共享状态，调用协议错误由 worker catch 回传。

### Part 2：先收集结果句柄，再移交关闭批次

对应 `part2_and_3_batch_queue()`。主线程创建六个 int 任务和一个 string 任务，先保存每个内层 future，再用 packaged_task<void()> 包装并入队，最后创建唯一 worker 排空。

这个基础版本采用关闭批次：queue 动态分配、无固定容量；push 成功才算接受；启动 worker 之前提交阶段结束；worker 执行期间主线程不再触碰 queue；join 后才检查排空和计数。空队列就是结束，无在线 close 接口，也不允许并发提交。Part 5 再切换到在线提交场景，保留旧题的并发队列必做内容。

### Part 3：只移动封装、异构返回和异常

拥有 packaged_task 的 lambda 不可复制，不能直接装入 std::function。Reference 用 static_assert 检查这一点，再实际构造 packaged_task<void()>。int 结果依次对应 0、100、200、指定失败、400、500；另一任务拥有移入的 unique_ptr 并返回 item-7。

ID 3 的异常为 job_failure，消息 job 3 failed。Reference 逐个检查 ID 对应值与预期失败，并检查外层 envelope 的 future，避免包装器把调用协议错误藏进无人消费的第二层通道。worker 循环自身异常进入 infrastructure_error，join 后先检查它，再取各任务结果。

### Part 4：一次调用与未执行就销毁

对应 `part4_one_shot_and_abandon()`。同一任务共享状态已完成后再次调用，抛 promise_already_satisfied；第一次结果仍为 42。另一个未执行任务销毁后，future.get 抛 broken_promise。任务不是构造时就执行，也不是开始调用的一瞬间就 ready。

### Part 5：在线提交、关闭后排空

对应 `part5_online_queue(0)`、`part5_online_queue(4)` 与 `part5_online_queue(4, true)`。mutex 保护 queue 和 closed，worker 用谓词 `closed || !queue.empty()` 等待，移出任务后解锁再调用。close 置位并通知全部等待者；关闭且空才退出，因此已接受任务仍被排空。关闭之后 enqueue 返回 false，参数对象销毁；持有其 future 的调用者会按放弃语义收到 broken_promise。分配失败通过异常报告，不计为接受。

验收使用固定交错：worker 先发送 entered，再经过 gate 才能访问队列；主线程收到 entered 后提交四个任务、调用 close，在锁内检查队列仍有四项，并检查四个结果都未就绪。gate 尚未释放，所以这个积压状态不受 worker 调度快慢影响；握手只证明到达启动边界，不推断线程正停在 wait 内的哪条指令。

随后放行，任务函数在真正执行时记录 ID。join 后要求记录恰为 `0,1,2,3`，再逐项检查 0/10/20/指定异常。FIFO 的证据来自执行日志，不能由按提交顺序遍历 futures 推出。零任务版本同时检查空关闭可结束。关闭后另投一个任务，保留它的 future，检查立即就绪且 get 抛 broken_promise，拒绝不能只核验一个 bool。

第三次调用在放行之前注入 submission_failure。生产者 catch 保存原异常并 close；release promise 位于比 worker 更内层的作用域，先析构并使 gate 状态 ready，随后才 join。worker 只 gate.wait、不 get，所以放弃事件也能解除等待。验证仍要求四项全部排空、执行顺序正确，再核验注入异常确实回到调用方；其他检查/分配异常在 join 后重抛，不当成预期失败。不得在该内层作用域尚未退出时先 join，否则 release 还没销毁就形成等待环。

worker 失败由专属错误槽在 join 后重抛。此版本无固定容量限制、单生产者/单消费者 FIFO、阻塞等待，不宣称无锁或固定完成时间；锁与条件变量完整推导在后续章节继续。当前测试固定的是关闭前积压与关闭后排空，不声称覆盖所有在线提交交错。

## 自测与验收

为什么先 get_future 再投递？这是所有权协议，避免 task 移入队列后难以再访问；若移动后的目的对象仍由你独占，标准没有禁止在那里 get_future。为什么 get 顺序不代表一般执行顺序？结果句柄保留身份，调用者可以任选收集顺序；本题的 FIFO 调用来自唯一 worker，不能推广到多 worker。

Reference 检查关闭批次的七个任务、结果身份、业务失败、异构类型、外层通道、重复调用与放弃，以及在线队列的空关闭、确定积压、实际 FIFO、拒收后的 broken_promise 和 gate 错误路径收尾，最后打印 `D3_reference OK`。不写线程池、不做性能排名；这是标准库接口与所有权练习。

规范：[packaged_task](https://eel.is/c++draft/futures.task.members)、[std::function 的复制要求](https://eel.is/c++draft/func.wrap.func.con)。C++23 move_only_function 是只移动调用包装的另一种选择，但本题真实运行的方案是 packaged_task<void()>。固定版本说明见正文 N5050。

## 构建与运行

从 `C08_Concurrency/exercises` 执行（VS2026 生成器需要 CMake 4.2+）：

```powershell
cmake -S D3_packaged_task -B build/D3_packaged_task -G "Visual Studio 18 2026" -A x64
cmake --build build/D3_packaged_task --config Release
./build/D3_packaged_task/Release/D3_packaged_task.exe
ctest --test-dir build/D3_packaged_task -C Release --output-on-failure
```

统一构建注册的检查目标是 `D3_packaged_task_reference`，CTest 使用进程级超时。
