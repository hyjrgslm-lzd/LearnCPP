# L02 部分写、背压与关闭

先完成 [L01](../L01_framing/README.md)，再读[主讲样章](../../chapters/03-framing-backpressure-shutdown.md)。你的独立编辑位置是 `student/solution.hpp` 的 `exercise::queue`。

| Part | 任务 | 已提供与检查 |
|---|---|---|
| A | 实现 FIFO 拥有消息与队首偏移 | 同一 checker 先写2字节再检查 `cdef`；Reference/good 独立于 Student |
| B | 实现64KiB硬上限及48/32KiB高低水位 | 精确容量、超限拒绝不改变原状态、下降恢复、空队列无效完成 |
| C（观察） | 预测连接状态并解释关闭责任 | `states.cpp` 完整安全状态模型，`loopback.cpp` 真实非阻塞 socket 驱动 |

`front()` 是借用，只允许串行 writer 在对应消息仍存活时使用；`consume(n)` 要求 `0<n<=front().size()`，不合法时抛 `invalid_argument`。`bytes()` 计入尚未整体释放的消息，包括已经部分发送的队首。空消息不占用队列节点。

`enqueue(string_view)` 只借用本次输入，复制到自有精确长度字节数组，不能保存输入视图或继承一个巨大备用capacity。64KiB约束自有payload申请量，不包含容器/分配器元数据。超出上限返回 false 且不改变队列；分配异常也不得留下虚假的已使用计数。水位与硬上限是不同责任：即使调用方没有遵守暂停建议，硬上限仍要拒绝。

## 完整解析

A：保存自有字节数组、长度和偏移。成功发送 n 后推进偏移；到末尾才弹出消息。bad 漏掉这个区分，提前丢弃后缀；它必须被 `partial send retains unsent suffix` 这条检查拒绝。另用不同长度和内容的多消息逐段消费，验证FIFO；`bad_lifo`必须被该内容检查拒绝。

B：减法检查 `size > limit-held` 可以避免先加法再溢出；插入成功后更新 held。只有完整节点释放才减掉对应大小。达到 high 后暂停，未下降到 low 之前保持暂停，避免反复改变读取兴趣。

C：模型接收两段残帧，EOF 后保留响应，逐字节排空，再验证重复 drain。慢读者实验停止消费直到 high，然后验证恢复；它没有声称已触发某个 OS 的具体 TCP 窗口行为。真实实验按字节发送，但系统可能合并它们；比较最终 wire，随后观察 EOF，不能根据 send 次数猜 recv 次数。

观察题补充：把服务端每次 send 的 span 截短到一个字节，结果仍应一致；删除队首偏移则应出现失败。将半关闭换为立即关闭客户端，会改变场景，不能要求客户端仍收到完整响应。

```sh
cmake -S C11_Networking_Service_Design/exercises/L02_connections -B build/c11-l02 -DC11_TEST_STUDENTS=ON
cmake --build build/c11-l02 --config Release
ctest --test-dir build/c11-l02 -C Release --output-on-failure
```

Student 初始打印 UNFINISHED 并返回2。Reference、独立good和bad校准属于实现验证；states/loopback运行成功不代替你对 Part C 的预测和解释。
## IDE 工程入口

VS solution 中本题主入口是 `C11_L02_student`。学生只编辑 `student/solution.hpp`；`checks.cpp` 是共同检查器，Reference/good/bad 和额外 bad 控制保持独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成，`C11_TEST_STUDENTS` 只控制是否把未完成 Student 注册进 CTest。
