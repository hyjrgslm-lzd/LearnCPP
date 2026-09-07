# K1：显式加法、对齐和安全尾部

本题 main.cpp 分类为 OBSERVATION：运行给定基线并进行本程序实际列出的观察/检查。退出成功只说明这些检查通过，不表示下面全部实现、推导或测量 Part 已完成；完整答案与更广检查见独立 solution.cpp。需要实现的 Part 请在自己的函数中完成后对照 Reference，不把运行答案视为完成作业。

完整连续正文：[课程正文](../../topics/simd/02-explicit-vectors.md)。

先完成 [标量与布局起点](../../topics/simd/01-scalar-and-layout.md)。main 给五项普通循环基线；[solution.cpp](solution.cpp) 验证所有可用后端，算法共用 [simd_kernels.hpp](../include/concurrency_study/simd_kernels.hpp)。没有 xsimd 也能编译默认 C++23 程序。

## 必做 Part 与答案

1. 写出 W=4 的整批循环。每次加载两个完整四项范围、相加、写入四项；条件使用 n-i>=4，i 始终不超过 n。后端 sse2 使用真实 intrinsic，xsimd 使用固定 sse2 的 batch，原生接口按 N5050 宏隔离。
2. n 不是 W 倍数时处理尾部。n=5 时一批加一项。Reference 覆盖 n=0..35，与解析式 3i+1 比较；主测试还检查紧贴分配边界和偏移起点。
3. 用 alignas(16) 提供真实对齐；aligned 模式检查三段地址后才使用 aligned 指令。unaligned 只放宽 SIMD 对齐，不放宽长度和寿命。
4. 比较 scalar 尾与临时数组 masked 尾。SSE2 没有本例需要的故障抑制加载，先只复制有效元素到四项临时数组，再向量运算，只复制有效结果回去。N5050 使用 partial_load/partial_store，不对三项 span 调 unchecked_load。

## 观察与答案

默认 main 输出为 3、5、7、9、11。Reference 在每次 aligned 或 masked-tail 调用前重新填错误输出，前后保留边界哨兵；masked-tail 的七项输出全部检查，不复用上一版的正确值。故意跳过写入时，同一个检查器必须失败。更完整的内存检查在 runtime_numeric_test；哨兵只能发现写，读越界还需要范围推导与 ASan。

测量用 simd_bench 的 scalar/sse2/xsimd/std_simd/tail_mask，默认 size=262147。缺可选后端只跳过对应专项；显式只选缺能力的 variant 返回 77，不把 scalar 回退伪称为向量运行。

## 构建与运行

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S K1_simd_basics -B build/K1_simd_basics -G "Visual Studio 18 2026" -A x64
cmake --build build/K1_simd_basics --config Release
ctest --test-dir build/K1_simd_basics -C Release --output-on-failure
```

C++ 默认 23，cs::check 在 Release 中保持有效。可选能力缺失不阻止普通基线；平台及标准事实的官方链接、完整推导见本题对应正文。
