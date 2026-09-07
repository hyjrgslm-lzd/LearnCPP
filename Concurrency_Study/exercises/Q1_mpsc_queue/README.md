# Q1：MPSC 的单消费者分支与发布空隙

完整正文：[MPSC](../../topics/queues/04-mpsc.md)。实现是 [queue_versions.hpp](../include/concurrency_study/queue_versions.hpp) 的 mpsc_ring<T>，即 sequence_ring<T,true>。[solution.cpp](solution.cpp) 覆盖全部 Part。

## Part 1：从 MPMC 槽位协议裁剪消费者竞争

生产者仍 CAS 抢 enqueue；消费者独占 dequeue，所以无 dequeue CAS。每槽 sequence 仍要保留，它表达代次和发布状态，而不是只仲裁消费者。容量要求二次幂且至少 2，固定槽要求默认构造、非抛复制赋值和析构；任一旧观察不得跨越半计数范围的进展。

答案：删除消费者仲裁不等于删除消费者发布。读完后仍 release 写 sequence=p+C，生产者 acquire 后才可复用。

## Part 2：正确性与生产者顺序

Reference 先检查容量 8 的填满/失败/清空，然后四个生产者合计发送 12007 个唯一 ID。各生产者区间不重叠，前若干生产者接收余数。单消费者逐项核对各生产者的原顺序并检查完整集合。异常通过共享驱动取消并回传主线程。

Reference 还用 `copy_overload_probe` 检查可变源重载会抛异常的类型。入队参数本来是 const，出队显式使用 `std::as_const(slot->data)`，和 `is_nothrow_copy_assignable_v<T>` 的实际约束一致；检查不误调可变重载、槽位能复用、最终对象归零。

## Part 3：确定性预订空隙

让第一张票的持有者在发布前暂停，第二张票写完并返回。此时 pop 必须允许 false，输出保持 77；恢复后返回 10、20。Reference 在真实队列内执行 noexcept 暂停钩子，用原子信号确认预订位置，不用 sleep。

答案：不能把 false 解释成严格线性化空；不能把原子 is_lock_free 或快速失败视为成功传输的 lock-free 保证。消费者不能绕过前票，否则改变既定顺序及槽位协议。

## 构建与测量

从 exercises 执行已接入的叶项目：

```powershell
cmake -S Q1_mpsc_queue -B build/q1 -G "Visual Studio 18 2026" -A x64
cmake --build build/q1 --config Release
ctest --test-dir build/q1 -C Release --output-on-failure
```

也可在已初始化的 Visual Studio Developer PowerShell 中独立直编：

```powershell
New-Item -ItemType Directory -Force build/q1-direct | Out-Null
cl /nologo /std:c++23preview /EHsc /utf-8 /O2 /DNDEBUG /I include Q1_mpsc_queue/solution.cpp /Fobuild/q1-direct/q1.obj /Febuild/q1-direct/q1.exe
./build/q1-direct/q1.exe
```

基准 variant=mpsc，consumers 必须为 1；可与相同参数的 mpmc 比较单消费者裁剪成本。该分支不支持 SPMC。
