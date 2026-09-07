# K3：选择语义、NaN 与零符号

本题 main.cpp 分类为 OBSERVATION：运行给定基线并进行本程序实际列出的观察/检查。退出成功只说明这些检查通过，不表示下面全部实现、推导或测量 Part 已完成；完整答案与更广检查见独立 solution.cpp。需要实现的 Part 请在自己的函数中完成后对照 Reference，不把运行答案视为完成作业。

完整连续正文：[课程正文](../../topics/simd/02-explicit-vectors.md)。

本题名保留旧 ID，但 N5050 实现使用自由 select 与 load/store，不混入旧 TS 的 where/copy_from 写法。main 是 relu 基线，[solution.cpp](solution.cpp) 包含三种条件运算的手写答案表，使用共用 SIMD 内核。

## 必做 Part 与答案

1. absolute：普通 abs(-0) 应为 +0。x<0?-x:x 会保留 -0，不能当作完整替代。SSE2 清除符号位，xsimd 调 abs，原生分支专门将零选为 +0。
2. clamp：先选择小于 -1 的值，再选择大于 1 的值。quiet NaN 保持 NaN 分类，-0 保持符号，不擅自使用 NaN 行为不同的 min/max 代替。
3. relu：x>0 时取 x，否则取 +0。因此 NaN、-Inf、-0 都得到 +0，+Inf 保留。不要把数学 max 的模糊表述代替精确条件。
4. 分别验证 NaN 分类与零符号。NaN 不等于自身，必须用 isnan；-0 等于 +0，必须用 signbit 补充。Reference 取九项强制尾部，主测试还逐长度截断并覆盖 subnormal。

## 为什么不是逐通道短路

select 的实参先求值。select(mask,1/x,0) 不能保证零通道没有执行除法；memory mask 也不能撤回之前已越界的 load。错误访问案例只作推理题，默认程序不会运行 UB。

默认严格策略要求 FTZ/DAZ 关闭，不承诺 signaling NaN 异常标志或 NaN payload 保持。完整输入政策见 [精度正文](../../topics/simd/03-reductions-and-precision.md)。缺可选库时仍验证标量与主机 SSE2，不让整个 K 模块因缺 xsimd 而无法运行。

## 构建与运行

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S K3_simd_where_select -B build/K3_simd_where_select -G "Visual Studio 18 2026" -A x64
cmake --build build/K3_simd_where_select --config Release
ctest --test-dir build/K3_simd_where_select -C Release --output-on-failure
```

C++ 默认 23，cs::check 在 Release 中保持有效。可选能力缺失不阻止普通基线；平台及标准事实的官方链接、完整推导见本题对应正文。
