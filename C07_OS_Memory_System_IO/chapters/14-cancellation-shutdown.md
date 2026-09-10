# 14 取消与关闭：停止接受、请求收束、释放资源

## 关闭不是一个动作

系统 I/O 的关闭至少包含四步：停止接受新请求，通知或取消已接受请求，消费每个请求的最终完成，最后释放资源。把这些步骤压成一个 `close()` 或析构函数，会让错误藏在竞态里。

同步文件复制中，函数返回时没有未完成内核请求，释放资源相对简单。completion 模型不同：提交成功后，请求可能还持有 buffer、offset、OVERLAPPED、SQE 相关数据或业务状态。即使上层已经决定“不要这个结果”，内核仍要给出最终完成，用户态仍要取走并标记收束。

## accepted set 是生命周期依据

正确实现通常维护一个 accepted set：只有系统确认接受的请求才进入集合；每个完成包或 CQE 消费后，从集合移除。shutdown 的停止条件不是“调用过 cancel”，而是 accepted set 为空。这样可以处理三类竞态：

- cancel 在目标完成前到达：目标以取消错误完成。
- 目标在 cancel 前完成：目标成功，cancel 可能报告未找到或已完成。
- 只提交了部分请求：只等待已接受的 request_id。

这个集合也让失败路径清楚。如果某个 API 在请求接受前失败，可以直接返回错误；如果请求已经接受，返回前必须 drain，或者在受监督子进程中以明确诊断终止，避免展开仍被内核借用的栈对象。

L08 的默认练习正是这个 accepted set 校验题。它不调用平台 I/O，而是消费一份事件 ledger：

```cpp
read_accepted(29, 1);
cancel_submitted(31, 29);
target_completed(29, 0, operation_canceled);
cancel_completed(31, 29);
```

另一份合法 ledger 允许目标先完成，取消随后报告找不到目标。实现必须接受这两种竞态，同时拒绝丢失、重复、未知 ID 和错误顺序。

## 资源释放顺序

释放顺序从借用关系反推。先停止生产新请求，再取消或关闭能唤醒等待的资源，然后消费完成，最后释放 buffer/context/ring/port/handle。反过来做会制造 use-after-free 或丢 completion：例如先销毁 buffer 再等完成，或者关闭 ring 后还期待 CQE。

关闭底层 handle/fd 也不等于请求立即消失。Windows overlapped I/O 仍需要 completion packet；Linux 上某些阻塞 I/O 可能持有 open file description 引用直到完成。课程公共 `unique_fd` 不重试 `close`，但这不代表业务 shutdown 可以忽略请求层。

## 检查器应该拒绝什么

好的检查器不相信实现自报“drained=true”。它应消费真实驱动给出的 ledger，并构造缺失、重复、乱序或错误 request_id 的反例。至少要拒绝这些错误：

- 只记录正常 read 完成，忽略被取消目标的最终完成。
- 把 cancel 请求完成当作 target 请求完成。
- request_id 对不上，却只按完成数量通过。
- bytes 不等于实际 payload 长度。
- 平台 OFF 时把未注册 platform 当 SKIP 或 PASS 结论。
- 已启用平台时把能力缺失的 SKIP 当成主体算法通过。

L08 Reference 和 good 使用两份独立校验实现；bad 只检查正常 read，必须被缺失 target completion 的反例拒绝。platform probe 只是给同一个 checker 提供真实事件来源，不替代默认 ledger 反例。

## 与 C09/C10 的桥接

协程和 sender 会把完成转换成恢复或 completion signal，但不会删除底层生命周期。awaiter 里的 operation state、sender 的 operation_state、回调对象和 buffer 都要遵守同一条规则：accepted work must complete before context release。

这也是为什么 C07 在 C09/C10 之前主讲系统完成模型。后续课程可以讨论调度、结构化并发和组合取消，但如果本章的 accepted set 和 drain 条件不成立，上层抽象只会更难调试。
