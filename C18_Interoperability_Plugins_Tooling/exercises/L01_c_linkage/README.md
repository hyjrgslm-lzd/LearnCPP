# L01：真正的 C 调用方

先读[语言链接](../../chapters/01-c-abi.md)。本题起点是链接期绑定的最小 V0，不是最终动态插件协议。C11 的 `consumer.c` 与 C++23 的 `implementation.cpp` 分别编译，再由 C++ 链接驱动链接；把调用方也改成 C++ 会丢掉本题的证据。

## Part 1：实现字节转换

只编辑 `student/solution.hpp` 中 `solve`。提供的 ABI 声明、调用方、输入和检查器不需修改。输入为有效的独立连续缓冲区，输出至少同长；允许长度为零时两指针为空。将 a-z 转大写，其余字节保留。这里先固定有效内存前提，容量与错误协议在 L02 展开。

**解析：** `length` 是字节数；输入中的零不是结束符。ASCII 范围判断只对 0x61—0x7a 生效，高位字节不应经过依赖 locale 的转换。Reference 用范围运算，good 用字母查表；它们独立生成结果。

## Part 2：证明边界

单题构建后运行 Reference；检查构建日志中 `.c` 由 C 编译器处理。临时在构建目录复制题目，移除 C++ 定义的 C linkage，再重建，观察链接诊断；这项观察不修改原题。若定义之前仍包含带 C linkage 的声明，它会继承该 linkage，因此要在副本中同时移除声明对定义的影响，不能把“只删定义前 extern C 仍成功”解释为编译器忽略规则。

**解析：** C linkage 不等于动态导出，也不保证不同架构或编译器 ABI 互通。本题通过只证明当前工具链中 C 调用方成功调用 C++ 实现。

```sh
cmake -S exercises/L01_c_linkage -B build/l01
cmake --build build/l01 --config Release
ctest --test-dir build/l01 -C Release --output-on-failure
```

`L01_student` 默认编译但不加入成功测试集合；未完成会以检查诊断失败。`validation/bad` 在 NUL 处提前停止，必须被 `complete byte conversion` 拒绝；不能把它改成主动打印失败。
