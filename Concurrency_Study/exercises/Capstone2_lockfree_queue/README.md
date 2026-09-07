# Capstone2：有界队列的完整演进与契约核验

正文主线：[预分配与批量](../../topics/queues/02-bounded-and-batch.md)→[SPSC](../../topics/queues/03-spsc.md)→[MPSC](../../topics/queues/04-mpsc.md)→[Vyukov MPMC](../../topics/queues/05-vyukov-mpmc.md)。ID 保留旧名称，不能因此把全部版本称为严格 lock-free FIFO。

[solution.cpp](solution.cpp) 是完整 Reference；[queue_versions.hpp](../include/concurrency_study/queue_versions.hpp) 是正确性与基准共用的真实实现。main.cpp 默认实际复现发布空隙，已无 mutex 占位冒充最终 MPMC 的情况。

## Part 1：重新验证基线与 SPSC

容量 8 时验证 mutex 基线、预分配环、SPSC 的填满/失败/清空。SPSC 传输 10003 项并核对顺序。答案：同容量不代表同角色；SPSC 只能由一个生产者和一个消费者访问，基线允许 MPMC。

## Part 2：预分配与批量

把 10 项交给容量 8 的 mutex_ring，push_batch 返回 8；pop_batch 返回 8 且逐项等于 0..7。随后以 batch=3、3P/4C 传输 10003 项，核对每个 ID。部分接受后必须从未发送后缀继续，不能把 batch 参数当实际成功数。

双重载回归对 mutex 基线、mutex_ring 和 MPMC 使用 `copy_overload_probe`。const 复制赋值不抛、可变源赋值会抛；检查单项转移、批量部分接受、未填后缀和最终寿命计数。mutex_ring 的 pop_batch 通过 const 容器取元素，既选择正确的复制重载，也兼容加锁的 vector<bool>。另外有真实 bool 批量前缀检查，SPSC 两版的并发 bool 回归在 G3。

## Part 3：MPMC 状态机和两个 CAS

第 p 张票的 sequence 路径是 p（可写）->p+1（可读）->p+C（下一轮可写）。生产者 CAS enqueue，消费者 CAS dequeue，互不替代；两端 ticket CAS 可 relaxed，载荷交接由 sequence 的 acquire/release 承担。失败 CAS 后重读对应槽位，不沿用旧槽。

T 在固定槽中赋值复用，要求默认构造、非抛复制赋值和析构。取得票据后不能放弃或抛异常。支持移动专属类型需要另一个异常/生命周期协议，本 Reference 明确采用复制接口。

## Part 4：边界、回绕与失败语义

Release 下明确拒绝容量 0、1、3、6；容量 1 会把“已发布”与“下一轮可写”编码成同一值。8 位 Counter 的同一实现顺序跨越 16 个周期，另枚举半范围内模序比较；旧 ticket 活过完整周期的别名反例也有可运行检查。

必须假设有效旧观察不会跨越半个计数范围的推进。固定数组不释放槽位，仍不代表有限 ticket 永远不存在 ABA。try false 可能来自预订未发布或取得未归还，不能当作严格 empty/full。

## Part 5：真实拓扑、逐项完整性与历史

容量 2、4、64 下分别运行 3P/1C、1P/4C、3P/4C，合计每轮 10003 个 ID。单消费者检查每个生产者顺序，多消费者检查唯一完整集合。SPMC 是允许拓扑，不是宣称另造了专用最优 SPMC 算法。小规模调用/返回顺序由 [queue_history_test.cpp](../runtime_tests/queue_history_test.cpp) 独立验证。

暂停第一个生产者取得的 ticket，第二个 push 返回后 pop 仍 false；恢复后依次得到 10、20。这个安全、无 sleep 的反例直接否定“严格 FIFO false 与无锁成功进展同时成立”的旧表述。

## Part 6：计时与答案解释

独立 [queue_bench.cpp](../benchmarks/queue_bench.cpp) 支持 mutex、ring、batch、spsc、spsc-cached、mpsc、mpmc、ms。每进程单轮，外部 runner 暖机与五次采样；按成功元素计数，消费者本地累计，无每元素公共计数原子。具体分组、命令、计时边界与完整答案见[验证与基准](../../topics/queues/08-validation-and-benchmark.md)。

不要求某个版本加速。mutex 与 ring 是共同严格单元素契约；sequence 队列允许暂时失败，跨实现对照必须注明是在“失败重试直到完成”的共同工作负载下测量。MS 无界且实际分配回收，单列。

## 构建

```powershell
# 从 Concurrency_Study/exercises 执行
cmake -S Capstone2_lockfree_queue -B build/cap2 -G "Visual Studio 18 2026" -A x64
cmake --build build/cap2 --config Release
ctest --test-dir build/cap2 -C Release --output-on-failure
```

Release 的 cs::check 与 worker 异常传播均有效。原子 is_lock_free 为实测值，不能替代进展证明。全 SC 不会代替尚未发生的数据发布，简单加 atomic_wait 也不自动提供正确的关闭/通知协议。
