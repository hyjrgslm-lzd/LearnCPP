# D2：用事件关系检查 async 策略和最后引用释放

完整推导见 [03 第 6—8 节](../../chapters/03-threads-and-execution.md)。[main.cpp](main.cpp) 是 deferred 的安全起点；[solution.cpp](solution.cpp) 是全部必做内容。不要用 sleep、固定启动先后或指定加速倍数作为答案。

## Part 与答案

### Part 1：显式策略与默认观察

对应 `part1_policies()`。deferred 的零时长查询返回 deferred 且调用计数为零；wait 在等待线程执行任务一次，get 不重跑。显式 async 用 entered promise 发出“函数体已开始”的信号，再等待 release；主线程先收到 entered，检查任务仍未完成，开放门闩后 get 得到另一个线程 ID。

这是同步协议推导，不是 async 返回前必已执行的保证。默认策略只打印本次状态与线程 ID，不要求固定选择，亦不把实现扩展错误地归为某个强制结果。

### Part 2：临时句柄和保存句柄

对应 `part2_temporary_and_retained()`。不保存 async future 的循环在每轮完整表达式结束时释放最后关联，下一轮前任务已完成，所以普通 completed 可以逐轮核验。任务只做不抛异常的有界递增；实际业务应保留 future 并 get 处理错误。

保存三个 future 的版本先完成三个发起调用；每个任务的完成必须经过尚未释放的 gate，因此结果均未就绪。这里没有逐任务 entered 握手，timeout 不能说明任务是否已经开始，更不能定位它正在 gate.wait 内。随后放行，逐项得到 0、10、20。答案是“允许重叠未完成任务”，不能写成“一定三倍快”。

### Part 3：释放第一份与最后一份

对应 `part3_last_reference()`。async future.share 后复制为两个句柄，先清空第一份仍能继续打开 gate；清空最后一份后，普通 completed 必为 1。这里的释放通过赋值发生，说明等待边界不限于析构。

再对照普通 promise 的 future：它可以在设值前销毁，随后 provider.set_value 仍能执行。未触发的 deferred future 销毁也不会执行闭包。不能推广成“shared_future 析构从不等”或“所有 future 析构都等”。

### Part 4：两种策略的业务异常

对应 `part4_exceptions()`。async 与 deferred 都让被调用函数抛 task_failure；wait 不重抛存储异常，get 重抛指定消息并使普通 future 无效。调用方创建 async 本身时的异常属于另一个阶段。

## 退出协议与验收

保存 futures 的拥有者在 release promise 之前声明。异常展开时 release 先销毁，gate.wait 因 broken_promise 状态 ready 而返回；任务完成后才释放 async 句柄。gate 只用 wait，不用 get，故放弃也可作为展开时放行。修改声明顺序可能制造“join 等 gate、gate 等 join”的环。

Reference 输出四个 Part 与 `D2_reference OK`。进程级超时会抓住等待协议回归。程序不报告性能提升；临时 future 语义对照是定义良好的运行，不是开启 UB 的错误实验。

规范：[async](https://eel.is/c++draft/futures.async)、[共享状态](https://eel.is/c++draft/futures.state)。按正文说明用 N5050 固定条款核对，eel 为滚动页。

## 构建与运行

从 `Concurrency_Study/exercises` 执行（VS2026 生成器需要 CMake 4.2+）：

```powershell
cmake -S D2_async_policies -B build/D2_async_policies -G "Visual Studio 18 2026" -A x64
cmake --build build/D2_async_policies --config Release
./build/D2_async_policies/Release/D2_async_policies.exe
ctest --test-dir build/D2_async_policies -C Release --output-on-failure
```

统一构建注册的检查目标是 `D2_async_policies_reference`，CTest 使用进程级超时。
