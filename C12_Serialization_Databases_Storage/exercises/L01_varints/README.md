# L01：最短 varint 与事务式游标

正文：[01](../../chapters/01-representation-and-varints.md)。已提供 bytes、result、独立黄金输入与检查器；学生只编辑 student/solution.hpp。

- Part 1：逐组读取低 7 位，用试探游标；成功后一次更新调用者的 cursor。
- Part 2：处理十字节上限、最高位溢出、不完整输入和冗余编码；失败不消费输入。
- Part 3：先推导 300、UINT64_MAX 与 -1 的编码，再运行黄金输入、截断与 ZigZag 检查，说明本练习与真实 Protobuf 解析契约的区别。

Reference 调用课程的完整实现；good 使用独立读取循环，bad 故意接受冗余编码。Student 初始返回 unfinished 并失败。运行学生检查时单独设置 C12_TEST_STUDENTS=ON；检查器不能靠调用 Reference 或预填结果通过。

解析：最高有效位的位置先限制移位次数，第十个字节必须终止且载荷至多为 1；非首组以 0 结束意味着存在更短表示。复制 cursor 能在任何错误分支保留外层解析状态。ZigZag 的运算在无符号域完成，不能对 INT64_MIN 直接取绝对值。
