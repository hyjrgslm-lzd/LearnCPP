# L01 增量分帧

主讲：[从字节到连接](../../chapters/03-framing-backpressure-shutdown.md)。只编辑 `student/solution.hpp` 中的 `exercise::decoder`；Reference 位于 `reference/solution.hpp`，调用公共完整实现。`good/` 是独立的整头解析算法，`bad/` 故意漏掉容量限制。检查器只通过选中的 `solution.hpp` 调用你的实现。

| Part | 已提供 | 你要实现 | 有效检查 |
|---|---|---|---|
| A 头部 | `frame_contract.hpp` 的类型、编码器及输入上限 | 八位十进制增量读取，非数字/超限拒绝 | 非数字、4097 长度、fatal 错误后再输入 |
| B body | `frame_step` 表示错误、待续或完整拥有消息 | 保存残片，按声明字节数完成并重置 | 多种 chunk、空 body、内嵌零字节、最大帧、多帧合并 |
| C 结束 | `finish()` 接口 | 边界 EOF 成功，残帧 EOF 失败 | 不同头/体位置截断、完成后 EOF |

`push(char)` 每次消费一个输入字节；待续返回空 `optional`，空帧完成返回含空 `string` 的 `optional`。结果不能借用下一次输入可能改变的存储。`buffered()` 报告逻辑保留字节数；头和 body 总量不得超出 4104。错误之后无需重新同步，但后续调用必须继续失败。

解析器不负责 Unicode、RPC body 字段或 socket I/O。不要把空传输帧的成功当成 RPC 请求语法也合法。

## 解析

A：逐位检查再累计，或保存八字节后使用严格数字解析均可；在根据 body 长度分配前拒绝大于4096的声明。Reference 会提前拒绝已经超限的前缀，good 在头读齐时拒绝，checker 接受这两种合法策略。

B：头完成后只有 body 达到指定长度才输出；完成的字符串移动给调用者，再初始化下一帧。空帧在第八个头字节立即完成。字符串长度按字节计数，不能使用 `strlen`。否则 `a\0b` 会被截断。

C：没有任何残片时 EOF 合法；只要头或体未结束就是 `truncated`。非法长度与截断是不同失败原因。清空残片后假装正常 EOF 会隐藏传输故障。

## 构建

```sh
cmake -S C11_Networking_Service_Design/exercises/L01_framing -B build/c11-l01 -DC11_TEST_STUDENTS=ON
cmake --build build/c11-l01 --config Debug
ctest --test-dir build/c11-l01 -C Debug --output-on-failure
```

初始 Student 输出 UNFINISHED 并返回2；这是明确的未完成状态。完成后相同检查自然通过。关闭 `C11_BUILD_REFERENCE` 后仍可单独构建 `C11_L01_student`，不会链接答案。`bad_high_digits` 只解析低四位，必须被高位非零的大长度输入拒绝，不能将 `10000000` 当成空帧。
