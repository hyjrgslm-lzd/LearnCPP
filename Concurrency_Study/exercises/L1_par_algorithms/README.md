# L1：四种策略的真实合同

本题 main.cpp 分类为 OBSERVATION：运行给定基线并进行本程序实际列出的观察/检查。退出成功只说明这些检查通过，不表示下面全部实现、推导或测量 Part 已完成；完整答案与更广检查见独立 solution.cpp。需要实现的 Part 请在自己的函数中完成后对照 Reference，不把运行答案视为完成作业。

完整连续正文：[课程正文](../../topics/performance/02-execution-policies.md)。

main 演示普通 transform；[solution.cpp](solution.cpp) 验证 plain/seq/par/unseq/par_unseq 的可用分支，以及 for_each、sort 和普通回调异常。所有策略映射使用共用 numeric::map。

## 必做 Part 与答案

1. 输入 [-16,16] 内的小整数 float，映射 x*x+2，整数公式生成独立真值。测试空、单项、7 项与 4097 项。输出已分配且与输入分离，回调不读写共享状态。
2. 用 par for_each 就地把独立 int 元素乘二。不同元素各有一个写者，不需要锁；不要推广到 vector<bool> 的位代理存储。
3. 对包含重复值的整数数组排序，期望 {-1,-1,0,2,2,3,3}。带 key/payload 的非稳定排序不能要求相等 key 保留原顺序。
4. 区分异常：普通 for_each 的异常被 catch；标准策略回调未捕获异常会 terminate，seq 也一样，因此该错误情形默认不运行。尺寸检查与结果检查在主线程。
5. 解释 unseq 的交错和 par 的回退。等待另一个元素推进不构成合法完成协议；par 可能串行，不能靠 sleep 验证调度。

## 验收边界

Reference 验证功能，不检测是否真的创建线程。CS_HAS_PARALLEL_ALGORITHMS 表示接口/链接可用，实际运行并行度需另测。无能力时普通基线仍运行，可选策略跳过；没有把串行占位函数命名为 par。

seq 不等于禁止所有机器向量化；unseq 也不保证生成 SIMD。有关分配调用的例外和 vectorization-unsafe 定义按固定 N5050 正文核对，本题选择无分配回调以简化证明。

## 构建与运行

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S L1_par_algorithms -B build/L1_par_algorithms -G "Visual Studio 18 2026" -A x64
cmake --build build/L1_par_algorithms --config Release
ctest --test-dir build/L1_par_algorithms -C Release --output-on-failure
```

C++ 默认 23，cs::check 在 Release 中保持有效。可选能力缺失不阻止普通基线；平台及标准事实的官方链接、完整推导见本题对应正文。
