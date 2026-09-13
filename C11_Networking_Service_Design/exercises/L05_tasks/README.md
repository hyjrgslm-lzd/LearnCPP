# L05 任务接纳与终态策略

正文：[RPC 策略](../../chapters/10-rpc-service-policy.md)、[任务状态机](../../chapters/11-task-service.md)。本题先手动推进状态，不需要 Boost 或真实网络。

## Part A：接纳

已提供 spec/snapshot/limits、拥有记录的 registry 和检查器。只编辑 [student/solution.hpp](student/solution.hpp)中的 policy::admit。按契约依次处理输入验证、同键重用/冲突、新工作停止接纳、live 和 records 容量。key 1—64、有限 ASCII 字符；整数与预算范围见正文。不要通过放大 limits 逃避拒绝。

## Part B：终态

实现 policy::finish(stop_reason,failed)。若 owner 已记录取消/到期原因，应保留第一次原因；否则 failed 决定 failed/succeeded。不能一律 succeeded，不能因 stop 请求就提前减 live。registry 负责实际拥有和计数，学生策略必须被所有接纳与完成决策调用。

```powershell
cmake -S C11_Networking_Service_Design/exercises/L05_tasks -B build/c11-l05 -DC11_TEST_STUDENTS=ON
cmake --build build/c11-l05 --config Release --target C11_L05_student
ctest --test-dir build/c11-l05 -C Release -R '^C11_L05_student$' --output-on-failure
```

[Reference](reference/solution.hpp)采用 [task_policy.hpp](../include/c11/task_policy.hpp)，[checks.cpp](checks.cpp)直接实例化所选 Policy 的 registry。good/bad 作为检查器正反控制；初始 Student 明确 UNFINISHED。

## Part C：解释观测

在现有检查旁写出三条历史：满载时同键重试、running 取消后新接纳、终态 TTL 到期。预测 live/retained/snapshot，再运行 Reference 对照。

解析：同键同 spec 不增 live，在满载时仍返回原身份；不同 spec 冲突。queued 取消无需等待 worker，running 取消只写 stop_requested 并保留 live，finish 才释放。records 包含终态保留项，TTL 从 ended 计时，不能淘汰 active。id 耗尽明确失败，不能回绕复用旧身份。订阅初始快照与登记同一 owner 操作完成，慢消费者关闭订阅且不取消任务。
