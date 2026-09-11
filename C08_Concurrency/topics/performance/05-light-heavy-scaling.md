# 测量进阶：轻映射、重映射与规模之间的关系

同一个并行算法在一种输入上更慢、另一种上更快，并不矛盾。线程分派和初始化有成本，每个元素有各自的计算量，内存也有供给上限。本篇用 [L3/reference.hpp](../../exercises/L3_par_vs_seq_bench/reference.hpp) 的两个完整负载和三档规模，恢复这三个维度的连续实验。算法定义都在 [numeric_kernels.hpp](../../exercises/include/concurrency_study/numeric_kernels.hpp)，[L3/solution.cpp](../../exercises/L3_par_vs_seq_bench/solution.cpp) 有独立数值验证。

## 1. 两个负载分别计算什么

light 的输入是 [-16,16] 内的整数值 float，函数为 `x*x+2`。乘加及最终结果在当前范围内可精确表示，检查器用整数公式逐元素验证。每项只有一次乘法和一次加法，逻辑数据至少为一个 float 输入和一个 float 输出，共 8 字节；如果没有其他开销，算术强度约 1/4 FLOP/byte。

heavy 的输入是 `x=(i%65-32)/64`，范围 [-1/2,1/2]，包括两个端点和零。任务定义为在 double 中递推计算 96 阶几何多项式，再输出 float：

```cpp
inline float geometric_value(float x) noexcept {
    double term=1,sum=1;
    for (int k=1;k<=96;++k) {
        term*=static_cast<double>(x);
        sum+=term;
    }
    return static_cast<float>(sum);
}
```

一次元素调用的工作是这个多项式的 96 个非恒定项，循环不是隐藏的基准重复采样。每次进程仍只对整个数组执行一次 transform。回调不分配、不加锁、不写其他元素且不抛异常，五种策略都使用同一份实现。heavy 入口在进入标准策略前检查有限输入和范围，避免在回调里抛出输入错误；这次 O(N) 检查计入调用时间。

这是用于方法学比较的**合成计算负载**。若实际需求只是求 `1/(1-x)`，应直接使用闭式公式；若需求是这个特定几何多项式，也可以研究闭式或快速求幂的替代算法。这里固定递推算法，目的是隔离“同一个计算量在不同执行策略与规模上怎样变化”，并不宣称递推是应用求值的最省运算方案。改变求值算法应另建对照组，不把消除了大部分工作量的版本混称为线程加速。

## 2. 为什么 heavy 的答案不来自另一份同样的循环

多项式 `S96(x)=1+x+...+x^96` 的闭式是 `(1-x^97)/(1-x)`。检查器不再实现这条逐项递推，而使用无限几何和 `S∞=1/(1-x)`，并明确预算二者之差。因为 `|x|≤1/2`，

```text
|S∞ - S96| = |x^97/(1-x)| ≤ (1/2)^97/(1/2) = 2^-96。
```

实际生成的非零输入最小绝对值为 1/64，96 次乘法得到的最小非零项不小于 2^-576，仍在 double 正常指数范围内；零输入的后续项恒为零。所有项的绝对值总和不超过 2，输出在约 [2/3,2] 内，没有严重抵消或溢出。

double 递推的舍入误差可以用约 192 次运算的 gamma 界估算，绝对规模受 2 限制；最终转换 float 的舍入远大于这个 double 误差和 `2^-96` 截断界。Reference 使用保守界 `2*epsilon_float*abs(S∞)+2^-96`，还覆盖严格侧一次 double 除法求闭式的舍入。它逐项检查有限性和差值，不要求不同策略必须逐比特一致，也不通过调用 geometric_value 给它自己判分。

Reference 覆盖长度 0、1、7、65、257，以及超出范围和 NaN 的入口拒绝。每个策略调用前把 out 填成 -12345，这个值不属于任何有效答案；在非空输入上，先让同一个检查器观察未写输出，必须捕获失败，再执行实际内核。这样“后一版没有写，但前一版已经算对”不能混过去。

## 3. 先预测规模变化会影响哪些成本

可以粗略写出 `T ≈ V*N + H + N*C/P_effective + memory_cost`。V 是入口验证的每元素成本，H 是调度和首次调用初始化，C 是映射计算量，P_effective 是实际有效并行度。这个式子只是推导框架；不能用一个测得的耗时倒推精确线程数。

在 1024 项时，即使每项较重，整体任务仍可能太小，不足以摊薄 H。到 65537 项，heavy 的计算更可能超过调度成本，而 light 仍可能以流式读写为主。到 1048577 项，两份 float 数组总计约 8 MiB，缓存层次和内存流量的重要性上升；heavy 的串行验证 V*N 也会限制最大收益。

选择三个规模是为了同时覆盖固定开销主导、过渡和较大工作集，并不是预先规定必须在哪一档交叉。`par` 可以串行回退，`unseq` 也不保证向量化；light 源码本身还可能已经自动向量化。因此预测应写成待检查的问题，不能把某个倍数作为测试条件。

## 4. 用完全相同的外部协议采样

L3 接受 `--workload light|heavy`，默认 light；每个工作负载都接受 `--size` 和 plain/seq/par/unseq/par_unseq 这五个具体 variant。suite 区分 policy_map_light 与 policy_map_heavy，variant 保持策略原名，满足统一 runner 的同名行要求。

从 exercises 目录使用独立 L3 benchmark 及 Reference 路径：benchmark.cpp 的正式目标名为 L3_par_vs_seq_bench_benchmark，main.cpp 是学生实现任务，不用于性能采样。先按[本题 README](../../exercises/L3_par_vs_seq_bench/README.md)开启 TEST_STARTERS 和 BUILD_BENCHMARKS 并完成构建；所有编译、ASan、正确性检查结束后运行：

```powershell
$exe = (Resolve-Path 'build/l3-student/Release/L3_par_vs_seq_bench_benchmark.exe').Path
$check = (Resolve-Path 'build/l3-student/Release/L3_par_vs_seq_bench_reference.exe').Path
$numericRun = Get-Date -Format 'yyyyMMdd-HHmmss'
foreach ($workload in 'light','heavy') {
    foreach ($size in 1024,65537,1048577) {
        python tools/run_benchmarks.py --exe $exe --check $check `
          --variant plain --variant seq --variant par --variant unseq --variant par_unseq `
          --seed 42 --warmups 1 --samples 5 --timeout 30 `
          --output "build/l3-student/measurements/$numericRun/$workload-$size" -- --workload $workload --size $size
        if ($LASTEXITCODE -ne 0) { throw 'sampling failed; keep its report' }
    }
}
```

输出根目录必须是新的目录。外层工作负载/规模顺序固定，每组内部由 seed=42 生成可复现的版本随机顺序；run.json 保留完整命令、顺序、每个样本及源码和二进制 hash。外部先暖机，再收集五次独立进程样本，不在驱动里重跑。新进程仍包含自己的首次标准库运行时初始化，不能把外部暖机称为同进程线程池稳态。

分配、输入生成、填错哨兵和严格 oracle 检查不计时。一次 map 调用包含形状/定义域验证和 transform 本身。正式计时进程中没有输出日志或记录线程 ID 的锁；CSV 在线程工作结束并验证结果后输出。后台其他负载不能被这份协议彻底消除，应如实记录；本批次不会与自身编译重叠。

## 5. 怎样解释完整样本

正式三档、两负载、五策略采样应使用稳定源码的新构建和全新输出目录。若同阶段存在集成重建、后台构建或未排除的整机干扰，这组记录只能用于方法学观察，不是冻结源码的正式性能结论。读取时先比较同一负载、同一规模的全部样本和中位数/范围，再观察趋势；不能把 light 与 heavy 的耗时比值叫作算法加速比。

若 light 的几个版本范围高度重叠，可以报告当前证据不足以稳定排序；若 heavy 的 par 在小规模慢、大规模快，调度摊销是合理解释，但真实线程数或频率仍需额外测量才能确认。反过来，如果三档都更慢，结果同样有效，应检查调度成本、运行时、串行验证以及实际生成代码，而不是增加重复次数挑选有利结果。

`threads=0` 表示标准库后端实际线程数未测，公共 runner 已支持；plain/seq/unseq 的 1 表示调用线程。这个实验不设置硬件亲和，不声称测得了实际 worker 数或内存带宽。

## 自测与答案

**heavy 为什么可以用无限和判分？** 因为先给出了有限和与无限和之间的上界 `2^-96`，并把它加入误差预算。没有这个界，直接替换 oracle 就改变了数学问题。

**96 次循环是不是违反每进程单轮？** 不是，它们构成一次多项式求值的不同项。对整个数组重复执行五次然后只报告最小值，才是这里禁止的隐藏重复采样。

**为什么不能用同一递推再算一遍作为唯一真值？** 两份代码可能共享漏项和舍入错误。闭式与截断界提供了独立验证路径；错误哨兵则阻止结果复用掩盖漏写。

**大规模 heavy 有收益，就能建议所有业务加 par 吗？** 不能。真实应用的函数、粒度、数据位置、初始化成本和线程资源可能完全不同。本例只支持当前输入、实现和机器上的测量结论。

策略规范固定参照 [N5050 的并行算法条款](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)；生成代码需结合 [Microsoft 向量化报告](https://learn.microsoft.com/en-us/cpp/parallel/auto-parallelization-and-auto-vectorization?view=msvc-170)。多项式、独立误差推导与测量设计是本课程实验合同。
