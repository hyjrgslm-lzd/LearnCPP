# SIMD 04：确认执行路径，再讨论速度

完成前三篇后，代码已经有普通循环、SSE2、可选 xsimd 与 N5050 原生分支。现在需要回答两个不同问题：生成的代码确实做了什么；在当前输入规模下，哪部分成本限制了整个算法。检测到向量指令只回答第一个问题，不自动意味着较快。

可运行诊断源是 [vectorization_probe.cpp](vectorization_probe.cpp)，它的三个 noinline wrapper 调用课程已有的 `add_scalar`、SSE2 `add` 和 `dot_scalar`，避免重新手抄一份看似相同的内核。main 用 17 项小整数输入检查解析答案，诊断程序自身也是可运行检查。

## 1. 从编译器报告找到目标循环

在本机，从 `C08_Concurrency` 目录执行：

```powershell
./topics/performance/verify-numeric.ps1 -Diagnostics
```

该脚本调用指定的本机 MSVC 安装，使用 C++23 preview、`/O2 /fp:precise /Qvec-report:2 /FAs`，复审修订后的产物在 `exercises/build/numeric-review-p2-diagnostics/`，首轮诊断仍保留在 `numeric-author-direct-diagnostics/`。它是作者验证与本机复现实验脚本，其他机器应改用自己的 Developer PowerShell 和编译器安装路径；正常课程构建仍使用公共 CMake。

2026-09-08 本机 MSVC 19.51 的实际诊断中，`numeric_kernels.hpp` 的 add_scalar 主循环报告 C5001“循环已向量化”；dot_scalar 报告 C5002“循环未向量化”。SSE2 显式循环本来就由 intrinsic 表达，优化器不再对其做一次自动循环向量化，并不说明其中没有向量指令。必须根据函数和源码行定位，不能只统计整份日志中“已向量化”的次数。

对应汇编中，普通 add 与显式 SSE2 路径都可看到成批浮点加载和 ADDPS；点积普通折叠保留 scalar double 累加。这是**该诊断构建**的证据，不是所有 Release、所有编译器或快数学配置的永久结论。尤其本批次严格验证使用 `/fp:strict`，不能直接把 `/fp:precise` 诊断套到另一份二进制上。

若使用 GCC，可在自己的叶目标增加 `-O3 -fopt-info-vec-optimized -fopt-info-vec-missed`；Clang 使用 `-O3 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize`。编译标准、目标 ISA 和浮点选项应一起记录。失败原因可能是依赖、调用、别名、成本模型或严格浮点要求；先读报告对应循环，再决定调整数据布局还是算法。

## 2. 如何做一个真的“无自动向量化”对照

普通 scalar 源码不是无 SIMD 保证。如果研究自动优化效果，应构建同一内核的独立对照目标，并禁用相关自动向量化优化，再检查汇编。GCC 可使用 `-fno-tree-vectorize`，Clang 可使用 `-fno-vectorize -fno-slp-vectorize`。它们是编译器选项，不是 C++23 的语义开关；启用它们后，也不能期待编译器不用 SSE 寄存器做 scalar 浮点运算。

区分 scalar SSE 指令和 packed SIMD 指令很重要。例如 ADDSS/ADDSD 只计算一个标量通道，而 ADDPS/ADDPD 计算 packed 通道；“看到 xmm 寄存器”不足以证明批量向量化。还要检查每次处理多少元素、循环步长和尾部路径。

本课程不默认禁用普通循环优化，因为普通优化后 C++ 是工程上合理的基线。无向量化目标是研究分解成本的额外实验，应单独命名与记录，不能把刻意削弱后的结果当成唯一 baseline。

## 3. ISA 能力和二进制能力必须同时成立

本课默认 x64 SSE2，并将 xsimd 固定为 `batch<...,xsimd::sse2>`，不会在普通路径上要求 AVX2/AVX-512。非 x64 构建不会包含这些 intrinsic。原生 std::simd 的实现可能依目标编译选项选择机器指令，必须把能力探测和目标 ISA 一起管理。

如果未来添加 AVX2 分支，不能只在 main 中检查 CPUID，然后用 `/arch:AVX2` 编译整个文件，声称检查前的代码一定安全。编译器可能在初始化、普通 helper 或检查逻辑中就生成高级指令。正确的分派设计通常把基线 dispatcher 与高级 ISA 内核分开编译，先检查 CPU 能力以及 OS 对向量寄存器状态的支持，再调用该内核；跨文件优化也要避免把高级指令内联回基线。

仅 CPUID 的硬件位还不完整：AVX 还涉及 OSXSAVE/XGETBV 所反映的操作系统状态管理支持。不得在不支持的机器上“试着执行看是否崩溃”。本批次没有新增未经验证的 AVX 目标，unsupported 后端通过 available 检查，显式选择时诊断 stderr 并返回 77。

ISA 回退有两层。编译时不支持某个库接口，`#if CS_HAS_XSIMD` 或 `#if CS_HAS_STD_SIMD` 隔离代码；运行时没有选定目标所需硬件，则不能执行该目标。一个能力宏表示库代码编译通过，不等同于对所有部署机器的运行时能力检测。

## 4. 一条 roofline 式的上界推导

设机器可持续带宽为 B 字节每秒，运算内核峰值为 P FLOP/s，算法算术强度为 I FLOP/byte。简化上界为 `min(P, B*I)`：计算单元再快，也不能超过按输入流量供给的速率。这是用于提出假设的模型，不是测出来的性能，更没有计入所有缓存和调度细节。

加法的逻辑 I≈1/12；double 累加的 float 输入点积，每元素两次 FLOP、8 字节读取，逻辑 I≈1/4。GEMM 若有效复用分块，单次加载可参与多个输出运算，算术强度比流式加法高得多。于是同一台机器上，加宽 SIMD 对三种算法的收益可能截然不同。

对于一次加法样本，可计算逻辑有效带宽 `12*n/(t_ms*10^6)` GB/s。它不是 DRAM 总线实测带宽：输入可能已驻留缓存，输出可能触发 write allocate，dirty cache line 稍后写回。若这个逻辑数大于内存条标称带宽，不应立即宣布测量不可能；先判断数据是否来自缓存。

同样，多个核流式读同一批数据可能争夺共享带宽。`par_unseq` 同时许可多核和向量化，但不能突破瓶颈上界；更宽向量还可能改变频率和功耗。需要观察到具体证据才能确认原因，不能把“AVX 一定降频”当成跨机器定律。

## 5. 把规模扫描与后端比较拆成可复现样本

先构建 benchmarks 叶项目，运行对应 Reference。之后从 exercises 目录使用统一 runner，例如：

```powershell
cmake -S benchmarks -B build/simd-leaf-bench -G "Visual Studio 18 2026" -A x64
cmake --build build/simd-leaf-bench --config Release --target simd_bench
python tools/run_benchmarks.py --exe build/simd-leaf-bench/Release/simd_bench.exe `
  --check build/k1/Release/K1_simd_basics_reference.exe `
  --variant scalar --variant sse2 --variant xsimd `
  --output build/results/simd-add-small -- --size 262147
```

这里使用独占的 `build/simd-leaf-bench` 叶构建目录。公共 bench preset 使用 exercises 根源配置，不能与 `-S benchmarks` 复用同一个 binary directory；整仓构建的可执行位置可能多一层 benchmarks，应传该 preset 的实际路径。再用新的输出目录测试 `--size 4194307`。尾数保留 3，是为了让主循环与尾部都实际执行；规模变化时不能悄悄只选择恰好整除的长度。

点积组使用 dot_scalar/dot_sse2/dot_xsimd/dot_std_simd，AoS/SoA 组使用 aos/soa。所有具体 variant 都只 emit 同名记录，不在同一进程偷偷执行另外一个版本预热。default all 只是手工冒烟方便，外部 runner 必须选择具体名称。诊断和跳过消息在 stderr，CSV stdout 只含 header 和有效 rows。

每组保留五个正式样本、中位数和范围。小规模测到非零启动成本或时间分辨率边界时，按原样记录；不要增加 hidden repetitions 来把它变成一个不可比较的“每次平均值”。真正要研究反复调用的稳态时，定义新的批调用合同并向外层报告真实完成量。

## 6. 验证配置与性能配置分开

`verify-numeric.ps1` 默认 `/O2 /fp:strict`；加 `-AddressSanitizer` 后使用 `/fsanitize=address` 验证内存访问。ASan 改变代码布局、指令和分配行为，所以它的时长只能作为测试运行记录，不参加发布性能排名。严格无 ASan 二进制才用于本批次外部采样。

该脚本的 Reference、观察程序、Starter、fast 专项和驱动执行都委托现有 `tools/run_benchmarks.py` 的 run_process，并设置默认 30 秒进程外超时，覆盖算法执行和析构收尾。原始退出码与 TIMEOUT/清理错误分别报告：77 是跳过，异常退出是失败，两个未完成 Starter 的已知退出 1 也不计为 Reference 通过。`verify-numeric.ps1 -RunnerSelfCheck` 使用同一薄适配器验证 0、77、7，并让受控进程分别在执行中和退出回调中阻塞于 Event.wait；外部超时必须判失败，不能与“预期 Starter 退出 1”混淆。进程管理和终止逻辑复用公共工具，没有另造框架。

`-XsimdInclude 路径` 启用固定库目录；`-FastMath` 只对独立内核单元启用 fast，oracle、误差界和比较单元始终严格编译并关闭跨单元 LTO，详见[精度篇](03-reductions-and-precision.md)。它运行专用有限域检查，不再把整个 numeric_test 或基准驱动按 fast 编译。这组结果也不混入默认严格测试通过数。原生 C++26 本机缺实现，仍需主线程保留公共探测日志；第三方 xsimd 的成功不填补这个证据缺口。

## 自测与答案

**报告说 SSE2 循环“未向量化”，是否说明它是 scalar？** 不是。自动循环向量化报告针对优化器变换，显式 intrinsic 已经表达 packed 操作。看实际循环与指令。

**普通循环自动向量化后，显式版本还有价值吗？** 可能用于控制加载、尾部、归约或数据表示，也可能没有额外收益。比较维护成本和实际测量，不能默认显式写法总更快。

**只检查 CPUID 再调用用 AVX 编译的整个程序安全吗？** 不足。检查前也可能执行高级指令，OS 状态支持也需要验证。分派基线和高级内核应按明确 ISA 边界构建。

**加法的有效带宽比内存规格高，必然计时错吗？** 不必然，可能来自缓存。需要区分逻辑字节与 DRAM 传输、工作集大小和计时范围。

## 官方资料

- [Microsoft 自动向量化与报告](https://learn.microsoft.com/en-us/cpp/parallel/auto-parallelization-and-auto-vectorization?view=msvc-170)。
- [GCC 优化选项](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)、[Clang 用户手册](https://clang.llvm.org/docs/UsersManual.html)：选项与生成代码应绑定编译器版本。
- [Intel 架构手册](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)：ISA 指令、CPUID、XGETBV 和系统状态前提。
- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：标准抽象的行为合同；性能、机器码和实际带宽均不是这份规范保证的结果。
