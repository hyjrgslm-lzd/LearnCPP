# Capstone3：实现 GEMM、归约与排序，再测量

完整推导见[并行计算项目正文](../../topics/performance/04-parallel-compute.md)。[main.cpp](main.cpp) 是 IMPLEMENTATION Starter，实际调用 student_gemm/student_sum/student_sort；[solution.cpp](solution.cpp) 是独立 Reference；[benchmark.cpp](benchmark.cpp) 是提供的实验驱动，正式目标名为 Capstone3_parallel_compute_benchmark。

学生 main 只复用输入生成与独立 oracle，没有执行参考内核冒充学生路径。默认未完成返回 1。通过小输入检查只说明这些检查通过，Reference/benchmark 的成功不能替代学生的全部实现与分析任务。

## 完成顺序

先完成必做实现 Part 1–6并运行默认学生检查；SIMD 是独立选做任务，完成后用 `--with-simd` 额外检查，不影响默认必做项的通过。随后阅读 Reference 的更广边界与异常检查，最后完成实验任务 A–C。若先观察提供的驱动，报告中明确标为观察，不计为学生实现完成。

## 实现 Part

1. **naive GEMM**：在 student_gemm 的 naive 分支实现 i-j-k，覆盖 C=A*B，不能依赖原 C。固定小整数数据有独立 int64_t 真值。
2. **tiled GEMM**：先清零 C，再分块；块内 i-k-j 连续更新，用 min 截断尾块，拒绝 block=0。
3. **threaded/par GEMM**：按不相交行分配工作；手写 worker 异常回传并 join，标准 par 回调不分配不抛出。不能让多个 kk 任务无同步地累加同一个 C。
4. **标准归约**：student_sum 的 plain/par 分支使用 transform_reduce，先提升 double；输入全 0.5，答案为 items/2。
5. **手写归约**：worker 私有局部和，结束时只写一次槽位，join 后汇总。不把单次槽位写入假定为必须 padding 的热点。
6. **排序**：student_sort 按选择执行普通或策略排序。检查同时验证顺序和元素重数，不能只看 is_sorted。
7. **选做 SIMD**：student_gemm 的 sse2 分支必须有真实显式向量内循环及 scalar 尾部；数值相等本身不证明已经用了 SIMD，需代码/诊断证据。

答案的核心在 numeric_kernels.hpp、simd_kernels.hpp，本题 solution.cpp 检查 n=0/1/3/7/17、block=1/4/32、旧 C=999、worker 异常与实际 CSV 线程元数据。学生 main 使用不同的 student_* 路径，只提供入门小检查，不能把 Reference 覆盖范围归给学生实现。

## 配置、构建和学生检查

以下 PowerShell 命令从 C08_Concurrency/exercises 执行，使用独占叶目录 build/cap3-student：

```powershell
cmake -S Capstone3_parallel_compute -B build/cap3-student -G "Visual Studio 18 2026" -A x64 `
  -DCONCURRENCY_STUDY_TEST_STARTERS=ON `
  -DCONCURRENCY_STUDY_BUILD_BENCHMARKS=ON
cmake --build build/cap3-student --config Release --target `
  Capstone3_parallel_compute Capstone3_parallel_compute_reference Capstone3_parallel_compute_benchmark

# 初始预期失败；实际执行 student_*，不是仅测 Reference。
ctest --test-dir build/cap3-student -C Release `
  -R '^Capstone3_parallel_compute_student$' --output-on-failure
```

填完后重建并检查两条独立路径：

```powershell
cmake --build build/cap3-student --config Release
ctest --test-dir build/cap3-student -C Release `
  -R '^Capstone3_parallel_compute_(student|reference)$' --output-on-failure
```

CTest 的超时覆盖计算与退出。默认 Starter 失败是尚未实现，不是 Reference 错误；只跑 Reference 不能声明学生 Part 完成。

选做 SIMD 完成后额外运行（仍先检查必做项）：

```powershell
python tools/run_diagnostic.py --timeout 30 -- ./build/cap3-student/Release/Capstone3_parallel_compute.exe --with-simd
```

未请求该选项时不会调用学生的 sse2 分支。显式请求但主机缺少该 ISA，且必做项已经通过时返回 77；实现尚未完成仍返回 1，不能以缺能力掩盖必做项失败。提供的 Reference 和 benchmark 会按可用能力检查/测量它们自己的 SIMD 实现，与学生是否选做是不同的事。

## 实验任务

**A：核对合同、完成量和线程口径。** 默认参数 size=48、block=16、threads=2、items=65537。GEMM size 是 n，completed 是 n*n；GFLOP/s 可按 2*n^3 推导，但计时含覆盖/清零和相应调度。手写 threaded/reduce_threaded 非空时报告裁剪后的计算 worker 数，details 另记 caller=1；空任务报告 threads=1、workers=0。标准库 par 系列的 0 专指后端线程数未测。

**B：用现有驱动分三组采样。** 编译与正确性/ASan 检查全部结束后执行下列完整流程：

```powershell
$exe = (Resolve-Path 'build/cap3-student/Release/Capstone3_parallel_compute_benchmark.exe').Path
$check = (Resolve-Path 'build/cap3-student/Release/Capstone3_parallel_compute_reference.exe').Path
$numericRun = Get-Date -Format 'yyyyMMdd-HHmmss'

python tools/run_benchmarks.py --exe $exe --check $check `
  --variant naive --variant tiled --variant threaded --variant par --variant sse2 `
  --seed 42 --warmups 1 --samples 5 --timeout 30 `
  --output "build/cap3-student/measurements/$numericRun/gemm" `
  -- --size 48 --block 16 --threads 2 --items 65537
if ($LASTEXITCODE -ne 0) { throw 'GEMM 采样失败；保留报告' }

python tools/run_benchmarks.py --exe $exe --check $check `
  --variant reduce_plain --variant reduce_par --variant reduce_threaded `
  --seed 42 --warmups 1 --samples 5 --timeout 30 `
  --output "build/cap3-student/measurements/$numericRun/reduce" `
  -- --size 48 --block 16 --threads 2 --items 65537
if ($LASTEXITCODE -ne 0) { throw '归约采样失败；保留报告' }

python tools/run_benchmarks.py --exe $exe --check $check `
  --variant sort_plain --variant sort_par `
  --seed 42 --warmups 1 --samples 5 --timeout 30 `
  --output "build/cap3-student/measurements/$numericRun/sort" `
  -- --size 48 --block 16 --threads 2 --items 65537
if ($LASTEXITCODE -ne 0) { throw '排序采样失败；保留报告' }
```

每个进程只运行指定 variant 一轮。不要使用 all 给外部 runner；不能把 main 的学生程序路径赋给 $exe。输出目录必须是新的；保存五个样本、中位数和范围，不把初始化/复制边界不同的 suite 混排。缺 ISA/库能力仅跳过相应专项。

**C：解释结果与限制。** 分块可能改善连续访问和复用，但块大小、工作集与环境决定效果；小 GEMM 的启动成本可能超过可分摊计算；SSE2 不保证四倍。一般浮点矩阵另由 numeric_test 使用每输出误差界验证，默认整数数据的严格相等不是对任意 float 的承诺。当前开发样本位于 [VALIDATION](../../topics/performance/VALIDATION.md)，处于集成构建未空闲环境，不能当作冻结版本正式结论。正式组由稳定源码的新构建运行。
