# Q2：使用真实 HP 回收的 Michael–Scott 队列

完整推导：[Michael–Scott](../../topics/queues/06-michael-scott.md)。实现是 [queue_linked.hpp](../include/concurrency_study/queue_linked.hpp) 的 ms_queue<T>，[solution.cpp](solution.cpp) 是全部 Part 的安全 Reference。

## Part 1：dummy、连接与帮助推进

初始化一个空 dummy，逻辑元素在 head 之后。push 在线性化到 next 连接 CAS 后成功，tail 更新失败不撤销节点，因为其他线程可以帮助推进。pop 在线性化到 head CAS 后移除队首，旧 dummy 被退休，next 成为新 dummy。

Reference 检查空失败不改变输出与 10、20 的顺序。T 可复制构造，复制赋值和析构不抛异常；节点复制构造/分配可在发布前抛出。不要求 T 默认构造，不支持只可移动元素。

## Part 2：保护与不可变 payload

enqueue 同时使用一个 tail HP，dequeue 同时使用 head、next 两个 HP。保护 head->next 后必须再次验证根 head，不能把旧 dummy 上永不更新的 next 当作足够的保护来源。所有源指针操作全 SC，符合课程 HP 契约。

答案：HP 保寿命，不给 payload 互斥。成功 pop 也不能 move/reset 共享 payload；本实现 const optional<T> 发布后只读，在 next 保护下复制到输出。CAS 失败不修改输出，也不直接解引用其写回的 expected。

const optional 的解引用已经是 `const T&`。新增双重载类型回归确认即使 `operator=(T&)` 会抛异常，MS 也只选择非抛 const 复制重载，并在作用域结束后释放全部该类型对象。没有改变原有 MS/HP 保护协议。

## Part 3：并发历史与实际拓扑

Reference 运行 4P/1C 的生产者顺序、1P/4C 的 SPMC 和 4P/4C 的逐项完整 ID，并触发多次运行期间的退休扫描。小规模严格 FIFO 历史由 [queue_history_test.cpp](../runtime_tests/queue_history_test.cpp) 补充，不能拿多消费者日志返回顺序代替线性化次序。

## Part 4：暂停保护者并证明延迟释放

慢消费者成功 CAS head 后暂停、尚未复制 10；快消费者移除 20 并退休保存 10 的节点。cleanup 后该节点不能析构。恢复慢消费者后仍读到 10，释放句柄并再次 cleanup 才允许析构。原子信号编排交错；若 worker 在到达暂停点前抛异常，也会通过 future 回传。

接着 1000 次非平凡 payload 传输检查运行中清理，只剩输入、输出、当前 dummy 三个对象；留下未消费节点后析构，最终 live=0。答案是实际回收，不是停止前一直泄漏或依赖进程结束。

## Part 5：完整操作与集成需求

经典连接/帮助核心在安全寿命与无锁原子前提下为 lock-free，完整 Reference 还包含 new、HP 槽获取、退休记录分配和域 mutex，不承诺完整无锁。无界分配失败是异常，不是满返回 false。

无需修改共享 HP 头：使用现有 make_hazard_pointer/protect/retire/hazard_pointer_cleanup；保护预算为每并发 enqueue 一槽、dequeue 两槽，默认全域 128 槽。停止并 join 后释放局部句柄，再析构与 cleanup。新增 CMake 交主线程，单源，无额外库，C++23。

## 构建

在 exercises 下执行已接入的叶项目：

```powershell
cmake -S Q2_ms_queue -B build/q2 -G "Visual Studio 18 2026" -A x64
cmake --build build/q2 --config Release
ctest --test-dir build/q2 -C Release --output-on-failure
```

在 Visual Studio Developer PowerShell 中也可直接验证：

```powershell
New-Item -ItemType Directory -Force build/q2-direct | Out-Null
cl /nologo /std:c++23preview /EHsc /utf-8 /O2 /DNDEBUG /I include Q2_ms_queue/solution.cpp /Fobuild/q2-direct/q2.obj /Febuild/q2-direct/q2.exe
./build/q2-direct/q2.exe
```

基准 variant=ms，要显式 --capacity 0。无界组单列，计时含逐次节点分配和热路径 HP 回收，不含最终析构；没有“不回收版本”参与有效排名。
