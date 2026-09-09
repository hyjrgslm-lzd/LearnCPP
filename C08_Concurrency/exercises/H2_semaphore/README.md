# H2：资源许可与双向交棒

先读 [正文](../../chapters/07-coordination.md)，再对照 [完整 Reference](solution.cpp)。本题默认 C++23，所有检查使用 Release 下仍有效的 cs::check；Starter 默认在启动并发前以 1 退出，学生实现的等待由外部 CTest 超时限制。

## 必做 Parts

1. 初始三份许可，八个任务各进入资源区 100 次。用 RAII 归还许可，检查占用不超过三、总完成 800、最终占用零。
2. 领完所有许可后检查 try_acquire 与定时尝试失败，再归还全部；核对 max() 至少达到模板要求。
3. 用 ready、ack 两个 binary_semaphore 做 100 次双向交棒，payload 使用普通 int，逐项核对内容。
4. 说明异常传播与握手责任：消费者即使发现数据不匹配，也必须先完成 ack 协议，再在主线程检查失败。

## Reference 对照与运行观察

`resource_limit()` 验证资源账本与上限；`handoff()` 验证 release/acquire 发布以及 ack 对下一次覆盖的约束。

## 学生入口与检查

[main.cpp](main.cpp) 提供独立 Starter，按中文 TODO 完成对应学生函数或方法；不要直接包含 solution.cpp。完成一个 Part 后把该项 `part*_done` 改为 true。标记仅解除启动保护，不代替验收：只有实际调用学生实现的检查通过才返回 0。未完成时输出 `STARTER INCOMPLETE` 并返回 1，发生在任何线程创建之前。

Part 1/4 实现 `student_with_permit`；Part 2 实现 `student_timed_acquire`；Part 3/4 实现 `student_send/student_receive`。检查 800 次工作不越过三份许可，抛出整数 17 后仍归还许可；100 次交棒的第 50 次故意让期望值不匹配，接收方必须返回 false 且仍完成 ack，不能让发送方挂起。

改动 main.cpp 后必须运行下面的 student 命令。Reference 的通过不说明学生实现正确。若只改标记、没有补全函数，TODO 或检查仍会失败；错误同步协议也可能被 CTest 的 30 秒超时终止。

不要求 try_acquire 在存在许可时一次成功，它允许虚假失败；本题把确定性失败检查放在无人能够 release 的零许可状态。

## 复盘答案

**模板 3 是实现的精确最大值吗？** 不是，least_max_value 是至少支持值；max() 才是实际上限。业务仍须遵守只归还实际领取许可的账本。

**峰值不到三为何通过？** 三是上限，不是调度承诺。串行调度同样正确，资源利用率需要另测。

**信号量可以由另一线程 release 吗？** 可以，它没有 mutex 那样的线程所有权。用它传信号正是这个特性的应用。

**有许可就能无锁改共享容器吗？** 不能，三份许可允许三个人同时访问；容器槽位归属仍需锁或其他协议。

**去掉 ack 会怎样？** 生产者可能在消费者读取前覆盖 payload。ready 只约束前向发布，ack 约束读取完成与下一次写入。

## 构建与验证

以下两套命令都在 `C08_Concurrency/exercises` 工作目录运行；Windows 示例使用 Release，构建目录分开以避免缓存选项混淆。

Reference 基准答案门禁（不运行 main）：

```powershell
cmake -S H2_semaphore -B build/reference-H2_semaphore -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-H2_semaphore --config Release --target H2_semaphore_reference
ctest --test-dir build/reference-H2_semaphore -C Release -R "^H2_semaphore_reference$" --no-tests=error --output-on-failure
```

学生实现验收（实际运行 main）：

```powershell
cmake -S H2_semaphore -B build/student-H2_semaphore -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-H2_semaphore --config Release --target H2_semaphore
ctest --test-dir build/student-H2_semaphore -C Release -R "^H2_semaphore_student$" --no-tests=error --output-on-failure
```

原样 Starter 返回 1，student CTest 报 Failed 是预期；不能把它标为 SKIP 或 WILL_FAIL 来制造通过。默认 `CONCURRENCY_STUDY_TEST_STARTERS=OFF`，未完成学生测试不进入 Reference 门禁。可选直接有界运行：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/student-H2_semaphore/Release/H2_semaphore.exe
```

成功只说明本次输入与实际交错通过相应检查；文字推导、一般交错、未测平台仍须单独核验。规范和完整答案见正文；不要求特定加速。
