# L02：借用缓冲与失败原子性

先读[所有权与错误](../../chapters/03-ownership-errors.md)。只编辑 `student/solution.hpp`；实现 `transform`。调用方提供内存，本题不转移所有权，输入/输出允许重叠。

## Part 1：定义检查次序

非空输入指针必须有效；容量不足返回 `too_small` 与所需长度，输出一字节都不变；足够容量时非空输出指针必须有效。空输入允许两指针为空，返回零长度成功。检查器会重置哨兵，不会让上次正确数据掩盖本次漏写。

**解析：** 先写若干字节再报告容量不足，虽然没有越界，却破坏“失败不提交”的保证。调用方重试时已无法假定旧结果存在。bad 正是这个行为错误：它正常执行，不使用越界或主动失败标记。

## Part 2：处理二进制与重叠

实现 ASCII 转换；保留 NUL、高位字节和输出尾部 canary。再使 `out = in + 1` 的重叠输入得到原始输入对应的结果。

**解析：** 正向逐字节读取和写入会覆盖下一步尚未读取的输入。Reference 先 `memmove` 再在输出内转换；good 先复制独立快照再转换。两者同契约，good 额外分配的成本不属于 Reference 的性能证据。不要用 `memcpy` 替换重叠搬运。

## Part 3：错误协议为何不是异常协议

本题 `Result` 是进程内部教学类型。后续 C ABI 使用固定状态码及 out 参数，不能把这个 C++ enum/class 直接当跨编译器 ABI。对地址可访问性的前提不能通过检查“非空”证明；这里只验证空指针/长度/容量契约，调用方仍须提供实际有效内存。

**解析：** 显式长度解决截断和边界计算，不解决悬空指针。跨语言包装还须持有输入 owner，直到原生调用结束。每一层负责不同事实，不能因为这里通过就省去 Python buffer release 或 Lua registry 引用。

```sh
cmake -S exercises/L02_buffers_errors -B build/l02
cmake --build build/l02 --config Release
ctest --test-dir build/l02 -C Release --output-on-failure
```

检查器按结果及未写区域验收；Release 不依赖 assert。未完成 Student 失败于容量协议检查；Reference/good 应通过，bad 应被精确的 `failure leaves output unchanged` 拒绝。
