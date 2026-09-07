# 队列 02：先减少持锁工作，再摊薄同步成本

[上一章](01-mutex-baseline.md)给了我们一个容量明确、失败语义明确的队列。现在假设它确实位于热点：每次只搬一个小整数，却反复进入临界区。我们先提出两个可分开验证的假设：动态容器管理是否增加了临界区工作？一次处理多个元素是否能减少加锁次数？本篇不假定它们一定更快，而是把这两个变化各自落到可以检查的代码里。

实现位于 [`queue_versions.hpp`](../../exercises/include/concurrency_study/queue_versions.hpp) 的 `mutex_ring<T>`；完整答案在 [Capstone2 的 solution.cpp](../../exercises/Capstone2_lockfree_queue/solution.cpp)。这里的基线本来就有界，新版本改变的是预分配存储方式，不是把无界问题突然改成有界问题。

## 1. 用逻辑位置描述固定数组

假设容量为 4，`head=1,size=2`，逻辑队列包含数组位置 1、2。下一次插入的位置是 `(head+size)%4=3`；下一次取出的位置是 `head=1`。移除后只需把 head 前进一格并将 size 减一。逻辑长度可以等于 4，所以这里不必像后面的 SPSC 一样留一格：size 在同一把 mutex 内更新，已经能够区分满和空。

初始化时 `vector<T>` 一次构造全部槽位。之后单元素 push/pop 只在已有 T 对象上做复制赋值，不再向底层容器请求一个新节点或新存储块。构造仍然可能抛异常；T 的赋值即使声明 noexcept，也可能在内部调用锁或分配器，因此“队列容器热路径无分配”不等于“任意 T 的完整操作无分配”。本篇测量用 `size_t`，避免把这些因素藏进结果。

接口保留 `try_push(const T&)` 与 `try_pop(T&)`。容量必须大于零，构造时还限制在 `size_t` 最大值的一半以内，以免计算 `head+size` 溢出。满时 push 返回 false，空时 pop 返回 false 且不修改输出。多个生产者和多个消费者都可以使用；所有槽位、head 和 size 都由同一把锁保护。

为了避免“队列状态已经改了，元素赋值却失败”的半完成操作，本实现要求 T 可默认构造，复制赋值和析构不抛异常。它比 Q0 对元素类型的约束更强，因此称为“对共同元素域保持单元素契约的改进”。它不接受只可移动类型。要比较类型泛化能力，就必须把这个差异列出来，而不能只比较整数测试。

这里的“复制赋值不抛”还必须精确到实际表达式。`is_nothrow_copy_assignable_v<T>` 检查的是把 `const T&` 赋给 `T&`。一个类型可以同时有不抛的 `operator=(const T&)` 和会抛的 `operator=(T&)`；直接写 `out = data_[head]` 时，普通 vector 的可变元素是 `T&`，重载决议会选择后者。即使 trait 为真，这条语句仍可能抛异常。对已经取得票据或已经移除节点的算法，这会留下未归还槽位、跳过退休，或越过 noexcept 导致终止。

实际实现现在写成 `values[n++] = std::as_const(data_)[head_]`，先以 const 容器访问元素。普通 T 得到 `const T&`，与检查的表达式一致；`vector<bool>` 的 const 访问则得到普通 bool 值，避免对临时代理调用 as_const。mutex_ring 的压位 bool 访问全部在同一把锁内，所以不需要为了独立槽位改存储。后面的 SPSC 没有这把锁，必须用真正的 T 数组，不能沿用这个特殊化。

[`queue_checks.hpp`](../../exercises/include/concurrency_study/queue_checks.hpp) 的 `copy_overload_probe` 正是这种双重载类型：可变源重载真实抛异常，const 源重载复制 ID 且 noexcept。`check_const_copy` 检查单项出入、跨圈复用、批量部分接受与未填后缀，以及离开作用域后存活对象归零。Capstone2 将它用于 mutex 基线、mutex_ring 和 MPMC；其他 Reference 覆盖 SPSC 两版、MPSC、Treiber 与 MS。检查不只断言 trait 为真，还实际经过所有相关接口。定义与 const 辅助函数参见 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) `[meta.unary.prop]`、`[utility.as.const]`。

## 2. 把一段输入变成一个已接受前缀

观察真实实现的 `push_batch(span<const T>)`：加锁一次，逐个复制元素，直到输入用尽或容量用尽，最后返回成功插入的数量。返回的是前缀长度，不是布尔值，也不是“全部成功或全部失败”。例如队列只剩两个空位，输入 `[10,20,30]`，返回 2 后只有 `[10,20]` 入队，调用者应从 30 继续。

```cpp
std::size_t sent = 0;
while (sent < input.size()) {
    sent += queue.push_batch(std::span{input}.subspan(sent));
    // 真实并发驱动在本次返回 0 时 yield，并检查取消信号。
}
```

这段循环要求存在会取走数据的消费者；不能在无人消费的满队列上独自执行它。完整驱动见 [`queue_checks.hpp`](../../exercises/include/concurrency_study/queue_checks.hpp) 的 `transfer`，其中每次按尚未发送部分重新生成前缀，不会因一次部分接受而重复发送前几个 ID。

`pop_batch(span<T>)` 对称地填充输出的前 n 个位置。未填充后缀保持原状。输入、输出 span 指向的对象由调用者持有，不能让其他线程同时改写它们，也不能让这些外部引用在调用期间失效。批次为零是合法的空操作；基准里的 batch 参数要求正数，是驱动的工作量约束。

整个批次在同一个临界区内执行，其他队列操作看不到其中间状态。因此可以把返回的前缀视为一个原子的批量转移，也可以把它展开成一段没有其他操作插入的串行单元素操作。单元素接口只是长度为 1 的 span 包装，正确性与批量接口来自同一个实现。

## 3. 少加锁的代价在哪里

如果每次确实搬到了 8 个元素，每元素分摊的加锁次数下降。但持锁时间也变长了：其他调用者必须等整个批次结束。队列通常只剩一个空位时，传入长度 8 也可能只接受 1，不能把参数 batch 当成实际批量。

本课程批量接口不等待“凑齐一批”：看到几个就尽量搬几个，所以不会自行增加凑批定时器。但上层若为了得到完整批次而先积累数据，就会引入提交延迟，特别是低流量时。吞吐与单个元素等待时间是两个观测目标；本基准只报告整轮传输耗时，不能据此宣称尾延迟改善。

两把锁是否更好？如果直接给 head 和 tail 各一把锁，却继续同时读写普通 size，就会失去本篇的同步依据。要改成双锁链式队列，必须重建连接节点的发布与空队列判定协议。这里没有这一步，因为预分配与批量已经提供两个独立、可测的变化。双锁算法可参考 [Michael–Scott 作者页中的 two-lock 版本](https://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html)，不要把“拆锁”当成机械改写。

## 4. 实验与应该看到的结果

在 `Concurrency_Study/exercises` 下执行：

```powershell
cmake -S Capstone2_lockfree_queue -B build/cap2 -G "Visual Studio 18 2026" -A x64
cmake --build build/cap2 --config Release
ctest --test-dir build/cap2 -C Release --output-on-failure
```

Reference 先对容量 8 的 mutex 基线与预分配环运行相同的填满、拒绝、清空序列，再把 10 个元素批量送进容量 8 的队列，检查接受数量是 8、返回值逐项是 0 至 7。随后用三个生产者、四个消费者传输 10003 个 ID，批量设为 3。这个总量有意不是生产者数的倍数，能检查余数分配是否正确。

这里的“通过”表示这些输入与实际交错符合检查。临界区保护范围才是任意合法交错的主要论证。批量和非批量的性能命令见[基准篇](08-validation-and-benchmark.md)：先比 `mutex` 与 `ring` 的单元素共同契约，再比较 `ring` 与 `batch` 的调用粒度；不得将不同 batch 的结果描述成同一接口下无条件的速度提升。

## 自测与答案

**预分配后为什么仍然可能阻塞？** 获取 mutex 仍然可能等待。T 也可能调用其他同步操作。try 只表示不等待未来的空间或数据。

**批量返回 2 后能重试整个输入吗？** 不能。前两个元素已入队，重试会重复它们。只能从未接受后缀开始，Reference 的逐项唯一性检查会发现这类重复。

**可否先在锁外读 size，再决定要不要加锁？** 普通 size 的并发读写是数据竞争；即使换成原子，它也只能提供一个可能过时的提示，最终检查和修改仍必须满足完整协议。本实现直接在锁内判断。

**为何不把这里的复制改成任意移动？** 当前数据槽一直存在，移动赋值可以成为另一种接口，但必须重新定义部分批量接受后哪些输入被移走、异常时接受前缀如何报告。现有正文与代码仅承诺复制接口，没有把这个问题藏起来。

mutex 的同步依据可查 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) `[thread.mutex.requirements]`；上面的数组索引、批量顺序和容量证明是对本课程代码的推导。
