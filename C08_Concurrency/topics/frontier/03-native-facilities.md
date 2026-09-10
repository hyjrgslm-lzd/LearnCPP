# 原生 HP、RCU、sender：探测和主体分开

C08 已有教学版 HP、RCU 和固定 stdexec sender 练习。它们能讲清协议，但不能替代标准库原生设施。F03 只回答一个问题：当前工具链是否已经能用标准头、特性宏、真实实例化和链接跑通最小主体。

## 1. 最小能力门槛

| 能力 | 头文件和宏 | 最小主体 |
|---|---|---|
| 标准 sender | `<execution>`，`__cpp_lib_senders >= 202506L` | `std::execution::just/then` 与 `std::this_thread::sync_wait` 返回 42 |
| 标准 HP | `<hazard_pointer>`，`__cpp_lib_hazard_pointer >= 202306L` | `make_hazard_pointer`、`protect`、`retire` |
| 标准 RCU | `<rcu>`，`__cpp_lib_rcu >= 202306L` | 默认 domain、`std::scoped_lock<std::rcu_domain>` 读区、`rcu_synchronize`、`rcu_barrier` |
| C++29 线程属性 | `<thread>` | `thread::name_hint<char>`、`stack_size_hint`、`jthread` 属性构造 |
| C++29 HP batch | `<hazard_pointer>`、`<span>`，`__cpp_lib_hazard_pointer >= 202606L` | `make_hazard_pointer_batch`、`clear_hazard_pointer_batch` |

头文件存在但宏缺失、宏存在但实例化失败、实例化成功但链接失败，都不是 PASS。

## 2. F03 的退出含义

F03 是原生能力主体集合，不是带 `main.cpp` 的普通练习，也不提供摘要占位可执行文件。标准主体拆成三个独立 CTest：

- `F03_std_senders_reference`：只判标准 sender。
- `F03_std_hazard_pointer_reference`：只判标准 HP。
- `F03_std_rcu_reference`：只判标准 RCU。

每个主体只在 `CONCURRENCY_STUDY_BUILD_REFERENCE=ON` 时生成。每个主体的宏为 0 时返回 77，因为这表示对应标准设施缺能力；宏为 1 后编译或运行失败就是该主体 FAIL。这样一个可用设施不会掩盖其他设施仍缺能力，也不会用摘要程序制造假 PASS 或假 SKIP。

这和 M2 的 stdexec 练习不同。M2 验证固定第三方实现；F03 只验证标准命名空间和标准头。第三方通过不提升 F03 的标准能力。

## 3. 下游引用

C09 可以引用这里区分“停止请求”和“停止完成”；C10 继续负责 sender 的完整 operation state、scheduler、environment 和 scope 设计；C11 服务观测需要在真实运行时上处理日志、取消、flush 与关闭；C13 性能课程只在原生设施可运行且有同口径 benchmark 后讨论成本。

本专题到此为止。没有标准实现时，完整讲义和 F01/F02 模型练习仍可学习；F03 标准主体保持 SKIP，不用教学模型或摘要占位程序冒充未来标准库。

## 4. 来源

- [N5050 固定草案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：`[saferecl.rcu]`、`[saferecl.hp]` 和 `[exec]`。
- [N5055 编辑报告](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5055.html)：N5054 是 C++29 工作草案，且接入 P2019R9 与 P3428R4。

