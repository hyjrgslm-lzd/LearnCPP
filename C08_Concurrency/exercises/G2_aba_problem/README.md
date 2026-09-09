# G2：安全复现 ABA，并验证有限标签的边界

完整正文：[Treiber 与 ABA](../../topics/queues/07-treiber-and-aba.md)。[reference.hpp](reference.hpp) 是固定活数组的逻辑模型，[solution.cpp](solution.cpp) 执行全部三部分；它不声称是一个通用标签栈。

## Part 1：原子 CAS 为什么错误地成功

初始 A->B->C，慢线程记录根 A 与旧 next B。快线程实际 pop A、pop B、重接 A->C 并 push A。慢线程 CAS(A,B) 成功，错误地把已移出的 B 又接成根。

答案由返回值与最终根检查，而不是只打印故事。节点用活数组下标表示，普通 next 的读取与修改由两个 release/acquire 信号排序；因此没有悬空访问、数据竞争或内存泄漏。B 还活着，不应把这个模型描述成释放后访问。默认运行安全，不需要开启故意 UB。

## Part 2：带标签比较同一段实际变化

将数组下标与 32 位版本打包为一个 uint64_t 原子。实际根变化是 (A,0)->(B,1)->(C,2)->(A,3)，慢线程的旧 (A,0) 比较失败，根保留为 A。Reference 检查相同拓扑变换的结果；不是只修改版本号来冒充 pop/push。

这里只压缩数组下标，不能截断真实指针来复用打包函数。is_lock_free 是运行时观测，uint64_t 不在所有平台都必然无锁。

## Part 3：版本也会绕回原值

两位版本经历 A->B->A->B->A 四次变化后回到零，旧 CAS 再次成功。答案是有限标签只在旧观察不会活过一个版本周期的前提下排除这种 ABA；宽位数增加余量，不等于数学上的永久根除。

HP 管节点何时可以释放/复用，标签管 CAS 比较什么。标签不能保护被释放的节点；HP 也不阻止应用主动把仍活着的节点重新接入并改 next。G1 的节点只发布一次、退休后不重插，正是额外协议。

## 构建

```powershell
# 从 C08_Concurrency/exercises 执行
cmake -S G2_aba_problem -B build/g2 -G "Visual Studio 18 2026" -A x64
cmake --build build/g2 --config Release
ctest --test-dir build/g2 -C Release --output-on-failure
```

Reference 不靠 sleep 安排时序，future.get 回传 worker 异常。strong 能去掉伪失败，不能识别值的历史；全 SC 也无法消除这个合法逻辑交错。
