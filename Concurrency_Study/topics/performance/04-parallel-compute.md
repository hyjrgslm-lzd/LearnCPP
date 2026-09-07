# 并行计算项目：让同一个 GEMM 连续演进

本项目把[缓存布局](01-cache-layout.md)、[策略合同](02-execution-policies.md)与 [SIMD](../../chapters/15-simd.md)合在一起。实现是 [numeric_kernels.hpp](../../exercises/include/concurrency_study/numeric_kernels.hpp) 和 [simd_kernels.hpp](../../exercises/include/concurrency_study/simd_kernels.hpp)，完整数据生成、校验和 CSV 驱动是 [Cap3/reference.hpp](../../exercises/Capstone3_parallel_compute/reference.hpp)，[solution.cpp](../../exercises/Capstone3_parallel_compute/solution.cpp)负责所有必做 Part 的 Reference 检查。

## 1. 先固定矩阵乘的行为

输入 A、B 为 n×n 的 row-major float 方阵，输出 C 覆盖为 A*B，不读取调用前 C 的值。输入矩阵可共享同一块只读数据，但输出不得与任何输入重叠。span 尺寸必须恰好为 n*n，乘法检查尺寸溢出；n=0 合法，block 必须大于零，手写线程数必须在课程限制内。

这里先选择方阵，是为了让布局推导清晰，而不是声称生产 GEMM 只有方阵。更一般的 leading dimension、转置选项、alpha/beta 以及子矩阵边界都需要新合同和新测试。当前项目不引入这些未使用接口。

算术合同是 float 乘法和累加，严格配置禁止任意重结合；不同实现允许在给定输入域下使用运算相关误差验收。默认基准特意用小整数 float，使所有中间和精确可表示，校验可以更严格：n≤512、每项乘积绝对值≤6，所以部分和绝对值不超过 3072，远小于 binary32 连续表示整数的界限。

## 2. v0：直接翻译数学定义

对每个输出 `C[i*n+j]`，计算 `sum_k A[i*n+k]*B[k*n+j]`。`gemm_naive` 使用 i-j-k 三重循环，局部 float s 从零开始，最后一次写回 C。A 沿一行连续读取，B 沿一列跨 n 个元素读取。

对大矩阵，B 的列访问会使相邻 k 使用远离的地址；一个缓存行带来的相邻列元素可能暂时没有被使用。这里能提出局部性假设，不能仅看源码就断言该运行一定 memory-bound：矩阵小到能完全驻留缓存时，瓶颈可能是依赖链、指令吞吐等。

v0 是接口基线，不是唯一真值。默认数据 `A[i,k]=(i+2k)%7-3`、`B[k,j]=(3k+j)%5-2`，校验器在独立 int64_t 算术中按数学定义计算每个结果。一般浮点数据的额外检查在 `runtime_tests/numeric_test.cpp`，使用更高精度的逐输出计算及误差界。这样优化版和基线即使共享某个错误，也更难互相掩盖。

## 3. v1：连续内循环和分块

先换观察角度：固定 i 和 k 时，`A[i,k]` 是一个可复用标量，对 j 扫描时，B 的第 k 行和 C 的第 i 行都连续。`C[i,j] += A[i,k]*B[k,j]` 不再跨列追逐 B。再把 ii、kk、jj 三个方向分块，限制一次反复使用的数据范围：

```cpp
for (auto i = ii; i < i_end; ++i)
    for (auto k = kk; k < k_end; ++k)
        for (auto j = jj; j < j_end; ++j)
            c[i*n+j] += a[i*n+k] * b[k*n+j];
```

外层循环和截断边界完整位于 `gemm_rows`。每次步进用 `min(block, remaining)`，避免把 n 不是 block 倍数的最后一个块漏掉。函数调用前清零 C，因为新结构会多次对同一元素执行 +=。Reference 在各轮之间把 C 填成 999，故遗漏清零会直接失败。

若以 B×B 的 A、B、C 三个局部块粗估工作集，float 数据约为 `3*B*B*4` 字节。例如 B=16 为 3072 字节，B=64 为 49152 字节。这个估算帮助提出候选块尺寸，但还没计入缓存相联度、其他数组、代码、预取和并行竞争，不是“必须占满 L1”的规则。block 是保留的实验旋钮，先用 1、4、32 验证，再按固定规模扫描性能。

v1 与 v0 都覆盖同一 C，均按递增 k 累积每个元素；循环次序和数据重用方式变化，输出合同没有变化。真实编译器是否进一步融合或重结合取决于浮点选项，必须把选项写入记录。

## 4. v2：让线程拥有不重叠的输出行

`gemm_threaded` 把行范围 [0,n) 分给至多 min(n,threads) 个 worker。每个 worker 只写自己的 C 行，对 A、B 只读。虽然不同 worker 会读同一 B 行，但没有共享写；不需要为矩阵每个元素加锁。

`parallel_chunks` 使用商和余数划分：前 `n%T` 个 worker 多一行。区间没有遗漏或重叠，空输入不创建线程；请求线程数仍先验证。主体不依赖另一个 worker 的中途状态，所有计算结束后 join，再读结果。异常捕获、存储、join 和重抛由公共函数完成，Reference 注入一个 worker 异常，确认调用者确实收到它。

CSV 的线程口径也区分这两种事实：非空手写版本以 `min(n,threads)` 报告计算 worker 数，details 另记 `caller=1`；空任务没有 worker，但调用线程仍完成检查和返回，因此记录 `threads=1, workers=0, caller=1`。手写归约按 items 做同样处理。这个已知的零 worker 不能编码为 0，因为公共 runner 的 0 专指库后端线程数未测。

`gemm_par` 是另一条真实实现。它预先创建行块编号，用 `std::for_each(std::execution::par,...)` 执行每个行块。回调只调用经过主线程验证的无分配内核，标记 noexcept；业务异常不能在这里套用 jthread 的传播合同。标准库线程数由实现决定，CSV 写 0；并行策略可回退，不能从函数名断言用了多少核。

若把每个 kk 块交给独立线程，多个线程会同时累加同一个 C 元素，原来的所有权证明就失效。可选的修复是各线程写私有部分矩阵，最后归约，但内存量、额外遍历和浮点分组都变了，需要新的实验。当前版本按行切分，避免引入这些额外协议。

## 5. v3：同一个连续内循环接显式 SIMD

`gemm_sse2` 保持分块结构，固定 a[i,k] 后广播到四个通道，加载四个连续 B 和 C，用 `_mm_mul_ps` 与 `_mm_add_ps` 计算，再写回 C。尾部仍按标量计算，因此 n 和 block 都不必是 4 的倍数。

本版本是真实显式向量内核，使用 x64 基线 SSE2，不把原来的 scalar 函数换个 vector 名字。它不使用 FMA，也不要求 AVX；相同 C 的覆盖合同保持不变。它目前是单线程显式版本，供观察线程并行与通道并行两个维度；不能将其结果标成“多线程 SIMD”。如果要组合两者，复用已经证明的行所有权，在每个行范围内调用该内核，并重新检查所有尾部和异常路径。

## 6. 两个对照：归约与排序

归约输入全部为 0.5f，解析结果为 items/2，double 精确可表示当前规模。`reduce_plain` 与 `reduce_par` 使用 transform_reduce 并显式转成 double；`reduce_threaded` 在每个 worker 的局部变量中累积，最后只写一次独占槽位，再由主线程合并。

这个槽位没有强行 padding。每个 worker 只写一次时，高频共享写已经被消除。是否有必要再增加每槽内存占用，应该单独测，不能把“所有多线程累积器都必须占一个缓存行”当成正确性规则。

排序基准为每个版本复制同一未排序输入，复制在计时外，计时只包含 sort 调用。输入值域为 [0,100]，校验先验证非递减，再比较 101 项直方图，证明每个值的重数不变。只使用 `is_sorted` 无法发现丢失数据；仅比较一个校验和也可能漏掉成对错误。

## 7. 构建、测量与验收

```powershell
# 工作目录 Concurrency_Study/exercises
cmake -S Capstone3_parallel_compute -B build/cap3-student -G "Visual Studio 18 2026" -A x64 `
  -DCONCURRENCY_STUDY_TEST_STARTERS=ON -DCONCURRENCY_STUDY_BUILD_BENCHMARKS=ON
cmake --build build/cap3-student --config Release --target `
  Capstone3_parallel_compute Capstone3_parallel_compute_reference Capstone3_parallel_compute_benchmark
# 未填 Starter 时其 student 测试预期失败；填完再检查两条路径。
ctest --test-dir build/cap3-student -C Release --output-on-failure
python tools/run_diagnostic.py --timeout 30 -- `
  ./build/cap3-student/Release/Capstone3_parallel_compute_benchmark.exe --variant tiled --size 48 --block 16 --threads 2 --items 65537
```

Reference 检查 n=0、1、3、7、17，block=1、4、32，并运行所有可用版本。benchmark.cpp 与 Reference 使用同一被测试实现，校验失败抛错或返回 1。main.cpp 则保留学生实现任务，真正调用 student_gemm/student_sum/student_sort，默认未完成失败。benchmark 目标已正式接线；完整学生完成顺序、$exe/$check 赋值及三组 runner 命令见[本题 README](../../exercises/Capstone3_parallel_compute/README.md)。

外部 runner 的 GEMM 对照选择 naive/tiled/threaded/par/sse2；归约选择 reduce_plain/reduce_par/reduce_threaded；排序选择 sort_plain/sort_par。三组分别写入结果目录。每次进程只运行指定 variant 一轮，矩阵尺寸默认 48，输入 items 默认 65537。小尺寸旨在快速、可靠地检查完整路径，不要求出现加速；更大规模的测量需要单独记录。

所有矩阵版本计时都包括生成 C 所需的覆盖/清零。线程版本还包含创建和 join，par 包含行块索引准备及可能的运行时初始化。输入生成、输出验证均在计时外。因此其数值是完整算法调用成本，不能与只测寄存器微内核的 GFLOP/s 直接比较。

## 必做 Part 的答案解析

1. v0 的完整 i-j-k 乘法在 `gemm_naive`，B 的访问步长为 n；验收由独立整数真值逐输出检查。
2. v1 的清零、六重分块和截断边界在 `gemm_tiled/gemm_rows`；n=7、block=4 会覆盖不整除的两方向尾块。
3. 手写线程与 par 两种路线都实际执行分配给 worker/回调的行块；安全理由是输出行不重叠，不是硬件碰巧没有发生冲突。
4. transform_reduce 的映射先提升到 double，初始值只贡献一次；全 0.5 输入有解析真值。
5. 手写归约只在结束时发布局部结果，不在共享槽位上逐元素累加；worker 异常 join 后重抛。
6. sort(par) 真正调用策略重载，校验同时覆盖顺序和元素重数。
7. 统一基准一轮一次，外部五样本，明确初始化与线程生命周期边界。
8. 校验失败是失败退出。计时结果只在校验通过后 emit；不以速度为通过条件。

## 资料与边界

[N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 提供算法、线程与数值库的规范合同；[Intel 手册](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)用于核对 SIMD 和平台缓存行为。块尺寸估计、行所有权证明与基准数据域是本课程自己的教学设计。本项目不是经调优的 BLAS 替代品，性能结论只能绑定实际规模、编译配置与机器。
