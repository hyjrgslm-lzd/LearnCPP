# G1：Treiber 栈的 CAS、保护与实际回收

完整正文：[Treiber 与 ABA](../../topics/queues/07-treiber-and-aba.md)。实现来自 [queue_linked.hpp](../include/concurrency_study/queue_linked.hpp)，main.cpp 是顺序观察程序，[solution.cpp](solution.cpp) 是所有必做 Part 的 Reference。

## Part 1：从串行 LIFO 到根 CAS

先预测 push 10、push 20、pop、pop 的结果，再解释成功操作在哪里生效。答案是 20、10，线性化点分别是 head CAS。push 的新节点在发布前私有，next 在发布后不变；pop 在读取 next 之前通过 HP 保护 head。Reference 检查空失败不修改输出以及两个成功值。

CAS 失败会改写 expected，但不授予该指针解引用权；pop 必须重新 protect。当前头指针使用全 SC，这是共享教学 HP 的契约，不能只改成 acquire/release 而忽略回收握手。

## Part 2：真正重叠的生产与消费

Reference 让 3 个生产者与 4 个消费者同时传输 12007 个唯一 ID，合并消费者私有记录后逐项检查集合。总数相等不足以证明不丢不重；一个丢失加一个重复就能骗过计数。并发 LIFO 的调用/返回解释由 [queue_history_test.cpp](../runtime_tests/queue_history_test.cpp) 另行检查。

worker 异常保存在各自 exception_ptr 中并发出取消，主线程 join 后重抛。线程创建失败同样会打开启动门，让已创建线程能够退出。

## Part 3：退休与析构

Reference 使用带 live 计数的非平凡元素，完成 1000 次入栈/出栈后主动 cleanup，确认只剩输入、输出两个对象；再保留一个未弹出节点，检查栈析构后 live 归零。答案不是泄漏：弹出节点 retire，仍在栈上的活节点在停止后删除。

每次 pop 需要一个 HP 槽，push 不解引用旧 head。完整调用包含 new、退休表分配与域锁，不能从 pointer.is_lock_free 为真推出完整栈无锁。T 可复制构造；复制构造可在发布前抛异常。复制赋值和析构被静态约束为 noexcept，不要求默认构造。

类型回归使用 `copy_overload_probe`：它的 const 源赋值不抛，可变源赋值会抛。pop 必须从 `std::as_const(old->value)` 复制，避免在已经摘除节点后误选抛异常重载而跳过 retire。Reference 检查可变重载调用次数为零，并检查出栈及非空析构后全部对象归零。

## 构建

从 C08_Concurrency/exercises 执行：

```powershell
cmake -S G1_treiber_stack -B build/g1 -G "Visual Studio 18 2026" -A x64
cmake --build build/g1 --config Release
ctest --test-dir build/g1 -C Release --output-on-failure
```

默认 C++23，Release 使用 cs::check，CTest 由公共配置设置超时。核心指针协议与整个含回收操作的进展保证见正文，原子属性由运行程序实测输出。
