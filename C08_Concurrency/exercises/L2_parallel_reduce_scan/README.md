# L2：归约、扫描与非交换操作

本题 main.cpp 分类为 OBSERVATION：运行给定基线并进行本程序实际列出的观察/检查。退出成功只说明这些检查通过，不表示下面全部实现、推导或测量 Part 已完成；完整答案与更广检查见独立 solution.cpp。需要实现的 Part 请在自己的函数中完成后对照 Reference，不把运行答案视为完成作业。

完整连续正文：[课程正文](../../topics/performance/03-reduce-scan.md)。

main 给 inclusive 前缀 1、3、6、10。[solution.cpp](solution.cpp) 是所有 Part 的完整 Reference，逐项检查扫描，而不只验证最终总和。

## 必做 Part 与答案

1. 对 int64_t 的 1..N 做 accumulate 与 reduce，init 同为 int64_t(0)。N 最大 4097，所有和不溢出，答案 N*(N+1)/2。reduce(seq) 仍不承诺左折叠。
2. 以显式 double 乘法映射实现点积，两数组 {1,2,3,4,5}、{2,3,4,5,6} 的答案为 70。类型提升在乘法前。
3. inclusive 第 i 项为 (i+1)*(i+2)/2；exclusive 且 init=10 第 i 项为 10+i*(i+1)/2。空输入不写，单项 exclusive 为 init。init 只作为整个算法的一次初始值，不是每块多加一次。
4. 用仿射变换验证 scan 顺序。依次为 2x+1、3x+4、x-2，前缀系数为 (2,1)、(6,7)、(6,5)。组合满足结合律但不交换；scan 保持输入顺序，reduce 不提供该保证。
5. 严格浮点下 (1e20-1e20)+1 得 1，1e20+(-1e20+1) 得 0。Reference 直接检查两种括号，不强迫某个标准库 reduce 选某一结果。

## 自测

只检查最后扫描值会漏掉什么？中间错误的前缀，甚至互相抵消的错误。为什么“整数加法结合”还要限制范围？有符号溢出不满足有效运算前提。为什么不能一律用 1e-6 相对误差？真值接近零与大范围抵消需要运算相关绝对尺度，见 SIMD 精度篇。

所有可用 par 分支真实调用标准策略重载；无能力时仍检查普通数值算法。严格关联示例不作为 fast-math 配置的保证。

## 构建与运行

从 `C08_Concurrency/exercises` 执行：

```powershell
cmake -S L2_parallel_reduce_scan -B build/L2_parallel_reduce_scan -G "Visual Studio 18 2026" -A x64
cmake --build build/L2_parallel_reduce_scan --config Release
ctest --test-dir build/L2_parallel_reduce_scan -C Release --output-on-failure
```

C++ 默认 23，cs::check 在 Release 中保持有效。可选能力缺失不阻止普通基线；平台及标准事实的官方链接、完整推导见本题对应正文。
