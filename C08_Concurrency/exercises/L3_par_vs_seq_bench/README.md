# L3：实现轻/重映射，再做跨规模实验

先读[策略合同](../../topics/performance/02-execution-policies.md)、[轻/重负载与三档规模](../../topics/performance/05-light-heavy-scaling.md)和[测量先修](../../chapters/12-measurement.md)。

本题有三条明确的路径：[main.cpp](main.cpp) 是 IMPLEMENTATION Starter，实际调用 student_light/student_heavy/student_map；[solution.cpp](solution.cpp) 是独立 Reference；[benchmark.cpp](benchmark.cpp) 是提供的实验驱动，正式目标名为 L3_par_vs_seq_bench_benchmark。main 不包含 solution，也不调用完整参考算法代替学生函数。默认 Starter 未完成返回 1；Reference 或驱动运行成功不能冒充学生 Part 已完成。

## 完成顺序

先完成下面的实现 Part 1–3，并运行学生检查；再阅读/运行 Reference 对照输入域、异常和边界；最后完成实验任务 A–D。可以提前观察提供的 benchmark，但须如实标为观察，不能计为学生实现完成。

## 实现 Part

1. **student_light**：输入为 [-16,16] 中的整数值 float，计算 x*x+2。main 用独立整数结果检查；默认返回 0 会失败。
2. **student_heavy**：在 double 中递推计算 sum(k=0..96,x^k)，输出 float；输入限定 [-0.5,0.5]。主检查使用独立的 1/(1-x)、2^-96 截断界和明确舍入预算，不调用同一递推判分。
3. **student_map**：对选定策略实际使用学生的 light/heavy 回调写输出，保证长度与不重叠合同，heavy 定义域在回调前验证。main 每个策略调用前重填错误哨兵，检查学生路径的真实输出。

完整答案在 solution.cpp 及 numeric_kernels.hpp。Reference 覆盖两负载、0/1/7/65/257 项、全部可用策略、无效定义域及 skip-write 故障。main 的小检查只验证其实际输入；Reference 检查的是提供的答案，不能替代对学生实现更广输入和真实并行/SIMD路径的核验。

## 配置、构建和学生检查

从 C08_Concurrency/exercises 执行以下 PowerShell 命令；此题独占 build/l3-student，不与公共 preset 目录混用：

```powershell
cmake -S L3_par_vs_seq_bench -B build/l3-student -G "Visual Studio 18 2026" -A x64 `
  -DCONCURRENCY_STUDY_TEST_STARTERS=ON `
  -DCONCURRENCY_STUDY_BUILD_BENCHMARKS=ON
cmake --build build/l3-student --config Release --target `
  L3_par_vs_seq_bench L3_par_vs_seq_bench_reference L3_par_vs_seq_bench_benchmark

# 初始未填 TODO 时预期失败；这条命令确实测试 main 的学生实现。
ctest --test-dir build/l3-student -C Release -R '^L3_par_vs_seq_bench_student$' --output-on-failure
```

填完实现 Part 后重新构建，再执行：

```powershell
cmake --build build/l3-student --config Release
ctest --test-dir build/l3-student -C Release `
  -R '^L3_par_vs_seq_bench_(student|reference)$' --output-on-failure
```

CTest 注册了进程超时，覆盖计算和退出。学生检查仍失败时先定位具体 Part；不要只运行 Reference 的过滤条件后宣布学生实现通过。

## 实验任务

**A：核对两个负载与计时边界。** light 为小整数 x*x+2，heavy 为 double 递推的 96 阶几何多项式。benchmark 的输入生成、错误哨兵和 oracle 在计时外，计时包含一次 map、形状/定义域检查及标准库调度。96 次循环是一次多项式的项数，不是重复采样。真实应用若只需要闭式函数值，应研究更便宜的求值方法；此处固定递推是合成计算负载合同。

**B：完成两负载、三规模的五样本。** 先停止自己的编译和 ASan 工作，确认目标已生成。以下代码给出真实路径赋值和完整流程：

```powershell
$exe = (Resolve-Path 'build/l3-student/Release/L3_par_vs_seq_bench_benchmark.exe').Path
$check = (Resolve-Path 'build/l3-student/Release/L3_par_vs_seq_bench_reference.exe').Path
$numericRun = Get-Date -Format 'yyyyMMdd-HHmmss'
foreach ($workload in 'light','heavy') {
    foreach ($size in 1024,65537,1048577) {
        python tools/run_benchmarks.py --exe $exe --check $check `
          --variant plain --variant seq --variant par --variant unseq --variant par_unseq `
          --seed 42 --warmups 1 --samples 5 --timeout 30 `
          --output "build/l3-student/measurements/$numericRun/$workload-$size" `
          -- --workload $workload --size $size
        if ($LASTEXITCODE -ne 0) { throw '采样失败，保留该组报告并停止' }
    }
}
```

每个具体 variant 每进程只运行一轮；外部暖机和五次采样、随机顺序、超时由公共 runner 管理。输出目录按批次分开，禁止覆盖已有数据。stdout 只有该 variant 的 CSV 行，诊断 stderr，77 是缺能力跳过。suite 区分轻/重负载，标准策略的 threads=0 表示后端线程数未测，公共 runner 已支持；不能用 hardware_concurrency 冒充实测线程数。

**C：解释完整样本。** 保留全部五个值、中位数和范围，比较同一负载同一规模的不同策略，再观察随规模变化。小任务中固定开销可能占主导；较大 heavy 更可能摊薄调度，light 可能受数据搬运限制。新进程仍可能包含自己的首次线程池初始化。出现加速不是验收条件，范围重叠也应如实记录。

**D：写清证据边界。** 正式报告应使用稳定源码的新构建及新输出目录；一次成功运行或 Reference 通过不能证明全部调度历史、真实 worker 数或峰值带宽。处于集成构建未空闲环境的样本只能作为方法学观察。
