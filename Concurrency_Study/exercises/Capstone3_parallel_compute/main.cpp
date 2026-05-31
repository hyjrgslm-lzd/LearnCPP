// =====================================================================
// 练习 Capstone3_parallel_compute：第三阶段结课 · 并行计算项目
//   对应文档：Concurrency_Study/17-第三阶段结课-并行计算项目.md
//
//   学习目标（综合阶段三 模块 J/K/L 的核心）：
//     - 把一个计算密集核心【按层叠加优化】，每层都重新计时 + 重新校验：
//         主线 GEMM（矩阵乘）：朴素三重循环 v0 → 缓存分块 v1
//                              → 并行分块 v2（std::execution::par，模块 L）；
//         对照 1 并行归约：std::transform_reduce(par) 标准版
//                          vs 每线程局部累加 + alignas 防伪共享 手写版（模块 J/L）；
//         对照 2 并行排序：std::sort vs std::sort(par)（模块 L）。
//     - 把【访存模式】当一等公民：朴素 GEMM 慢在对 B 跨行跳（stride-N），
//       分块快在让 BS×BS 的工作集驻留 L1/L2 缓存（cache blocking，模块 J）。
//     - 并行归约要【避免伪共享】（局部累加器各占一条缓存行，
//       alignas(std::hardware_destructive_interference_size)，模块 J），
//       且要懂【浮点归约不满足结合律】，并行求和与串行可能有末位差异（模块 L）。
//     - 用严谨的【加速比测量方法学】：预热、多跑取最小值、Release/优化、
//       volatile sink 防优化（防 dead code elimination，模块 L 练习 L-3）。
//
//   本项目刻意【自包含、零外部依赖】：只用标准库（MSVC 并行算法内置，
//   无需链接 TBB）+ 模块 J 的伪共享规避技巧。SIMD 部分靠【编译器自动向量化
//   + 手写 SIMD-friendly 循环】实现，【不依赖 xsimd / std::simd】；显式向量化
//   作为【进阶方向】指回模块 K（见文末 // TODO [进阶 1]）。
//
//   骨架说明：关键实现处用 // TODO [必做 N]: / // TODO [进阶 N]: 标记。
//   未填 TODO 处给了【最小占位实现】（多为串行/朴素版）以保证本文件在
//   MSVC(VS2026, C++20) 下可直接编译运行且结果正确。各 TODO 注释写清了
//   “真正该做什么”，并以注释形式附上参考实现——填写时把占位段替换为它即可。
//   注意：为便于直接观察，本骨架的 v0/v1 已给出可运行的真实实现（朴素 + 分块），
//         因此 v1 相对 v0 一上来就有可观加速；而 v2 的并行（必做 3）、两个归约的
//         并行/多线程（必做 4/5）、并行排序（必做 6）的占位仍是【串行】，其加速比
//         初始会接近 1.00x —— 这【符合预期】，换上 par/多线程实现后才会拉开。
//
//   官方参考：
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams)
//         第 10 章（并行标准库算法 / reduce / transform_reduce / 执行策略）；
//         第 8 章（8.2 数据布局对性能的影响、8.2.3 伪共享 false sharing）。
//     - std::execution（执行策略）:
//         https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t
//     - std::transform_reduce（并行归约）:
//         https://en.cppreference.com/w/cpp/algorithm/transform_reduce
//     - std::hardware_destructive_interference_size（缓存行 / 伪共享）:
//         https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
//     - 缓存分块 cache blocking / loop tiling 背景：
//         Ulrich Drepper, "What Every Programmer Should Know About Memory"（第 6 节）。
//
//   编译运行（VS2026, C++20；MSVC 并行算法内置，无需链接 TBB；务必用 Release/优化，
//   否则向量化被关闭、并行往往更慢，加速比与 GFLOPS 都不可信）：
//     cmake --build build-vs2026 --target Capstone3_parallel_compute --config Release
//     ./build-vs2026/Capstone3_parallel_compute/Release/Capstone3_parallel_compute.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <functional>
#include <iomanip>
#include <new>        // std::hardware_destructive_interference_size（C++17）
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------
// 缓存行大小常量（模块 J）。std::hardware_destructive_interference_size 是
// “为避免伪共享，两个对象至少应相距多少字节”的实现定义常量；x86-64 上
// 常见实现给 64。若实现未提供则保守回退到 64。
// ---------------------------------------------------------------------
#ifdef __cpp_lib_hardware_interference_size
constexpr std::size_t kCacheLine = std::hardware_destructive_interference_size;
#else
constexpr std::size_t kCacheLine = 64;
#endif

// =====================================================================
// 计时与防优化工具（方法学，复用练习 L-3 的范式）
// =====================================================================

// 跑 f() reps 次，返回最小耗时（毫秒）。最小值最接近“无被抢占干扰”的真实耗时，
// 比平均值稳定（平均值会被偶发的系统抖动拉高）。
template <class F>
double bench_min_ms(F&& f, int reps) {
    double best = 1e300;
    for (int r = 0; r < reps; ++r) {
        const auto t0 = std::chrono::steady_clock::now();
        f();
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        best = std::min(best, ms);
    }
    return best;
}

// 防优化用的全局 sink：把结果累进去，编译器就不能把“结果没人用”的计算整段
// 删掉（dead code elimination）。任何被计时的计算都应把某个结果喂给它。
volatile double g_sink = 0.0;

// 把一个浮点小数格式化成定宽字符串，便于对齐表格。
static std::string fmt(double x, int width, int prec) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(prec) << x;
    std::string s = oss.str();
    if (static_cast<int>(s.size()) < width) s = std::string(width - s.size(), ' ') + s;
    return s;
}

// =====================================================================
// 第一部分：矩阵乘 GEMM（主线）
//   方阵，float，行主序（row-major）：元素 (i,j) 位于 M[i*N + j]。
//   C = A * B，三个版本签名统一：v0 naive / v1 tiled / v2 par。
// =====================================================================

// ---------------------------------------------------------------------
// v0 朴素三重循环（基线）。i-j-k 顺序：
//   对每个输出 C[i][j]，沿 k 把 A 的第 i 行 与 B 的第 j 列点积。
//
//   访存模式（务必看懂，这是“为什么慢”的根）：
//     - A[i*N+k] 沿 k 顺序走（步长 1，缓存友好）；
//     - B[k*N+j] 沿 k 走时【每次跳一整行 N 个元素】（步长 N，跨行跳），
//       缓存极不友好——这正是朴素 GEMM 慢的主因（memory-bound）。
// ---------------------------------------------------------------------
void gemm_naive(const float* A, const float* B, float* C, int N) {
    // TODO [必做 1]: 实现朴素 i-j-k 三重循环（基线）。
    //   要求：先把 C 清零，再 C[i*N+j] += A[i*N+k] * B[k*N+j]。
    //   完成后请遮住下面参考实现重写一遍，并能口头解释：
    //   “内层 k 循环里，对 B 的访问 B[k*N+j] 为什么是跨行跳（步长 N）？”
    //
    //   参考实现（已启用以保证可编译运行且结果正确——这同时就是基线）：
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            C[i * N + j] = 0.0f;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            float acc = 0.0f;
            for (int k = 0; k < N; ++k)
                acc += A[i * N + k] * B[k * N + j];
            C[i * N + j] = acc;
        }
    }
}

// ---------------------------------------------------------------------
// v1 缓存分块（tiling / cache blocking）。把矩阵切成 BS×BS 的块，
//   六重循环：外三层 (ii,jj,kk) 按 BS 步进选块，内三层 (i,j,k) 在块内遍历。
//   每个块的工作集（A 的 BS 行片段、B 的 BS 行片段、C 的 BS×BS）足够小，
//   能驻留 L1/L2 缓存，于是 A/B 的数据被【复用】而非反复从内存重载，
//   缓存缺失（cache miss）大幅下降。
//
//   关键写法：用 i-k-j 的块内顺序（先固定 i、k，内层 j 顺序扫），
//   让内层对 C[i*N+j] 与 B[k*N+j] 都【顺序访问】，对自动向量化也友好。
// ---------------------------------------------------------------------
void gemm_tiled(const float* A, const float* B, float* C, int N, int BS) {
    // 先清零 C（分块版用 += 累加，必须先清零）。
    for (int i = 0; i < N * N; ++i) C[i] = 0.0f;

    // TODO [必做 2]: 实现六重分块循环（cache blocking）。
    //   要求：外层 (ii,jj,kk) 以 BS 步进；内层在 [ii,ii+BS) × [jj,jj+BS) ×
    //   [kk,kk+BS) 上做 i-k-j 顺序累加。注意块边界用 min 防越界（N 不必整除 BS）。
    //   校验：结果须与 gemm_naive 逐元素一致（容差内）。
    //   完成后遮住参考实现重写，并能解释“BS 该和 L1/L2 容量怎么对应”。
    //
    //   参考实现（已启用以保证可编译运行且结果正确）：
    for (int ii = 0; ii < N; ii += BS) {
        const int iMax = std::min(ii + BS, N);
        for (int kk = 0; kk < N; kk += BS) {
            const int kMax = std::min(kk + BS, N);
            for (int jj = 0; jj < N; jj += BS) {
                const int jMax = std::min(jj + BS, N);
                for (int i = ii; i < iMax; ++i) {
                    for (int k = kk; k < kMax; ++k) {
                        const float a = A[i * N + k];       // 块内复用：固定 i,k
                        const float* brow = &B[k * N];
                        float* crow = &C[i * N];
                        for (int j = jj; j < jMax; ++j)     // 内层顺序：对 C、B 都步长 1
                            crow[j] += a * brow[j];          // 自动向量化友好
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------
// v2 并行分块。在 v1 的基础上，把【最外层行块】分给多线程：
//   不同任务负责 C 的【不相交行区间】，因此对 C 的写互不重叠——
//   天然【无数据竞争、无需任何锁或原子】。这是“按数据划分 + 输出不相交”
//   这一最省心并行模式的典型应用。
// ---------------------------------------------------------------------
void gemm_par(const float* A, const float* B, float* C, int N, int BS) {
    for (int i = 0; i < N * N; ++i) C[i] = 0.0f;

    // 构造“行块起始行号”列表：0, BS, 2*BS, ...。每个元素代表一条要算的行块。
    std::vector<int> row_blocks;
    for (int ii = 0; ii < N; ii += BS) row_blocks.push_back(ii);

    // 算一条行块 [ii, ii+BS) 的全部输出（内部就是 v1 的分块内核，但 i 限定在本行块）。
    auto compute_row_block = [&](int ii) {
        const int iMax = std::min(ii + BS, N);
        for (int kk = 0; kk < N; kk += BS) {
            const int kMax = std::min(kk + BS, N);
            for (int jj = 0; jj < N; jj += BS) {
                const int jMax = std::min(jj + BS, N);
                for (int i = ii; i < iMax; ++i) {
                    for (int k = kk; k < kMax; ++k) {
                        const float a = A[i * N + k];
                        const float* brow = &B[k * N];
                        float* crow = &C[i * N];
                        for (int j = jj; j < jMax; ++j)
                            crow[j] += a * brow[j];
                    }
                }
            }
        }
    };

    // TODO [必做 3]: 把行块的计算用 std::execution::par 并行化。
    //   做法：std::for_each(std::execution::par, row_blocks.begin(),
    //                       row_blocks.end(), compute_row_block);
    //   为什么无需锁：每个 ii 对应的任务只写 C 的第 [ii, ii+BS) 行，
    //   不同任务的写区间不相交，没有数据竞争。
    //   完成后遮住参考实现重写，并能解释“若改成按列块划分，写 C 还安全吗”。
    //
    //   占位实现（串行 for_each，保证可编译运行且结果正确；初始加速比≈1.00x，符合预期）：
    //   —— 把下面这行替换成上面的 par 版本即可获得真正的并行加速。
    std::for_each(/* std::execution::par, */ row_blocks.begin(), row_blocks.end(),
                  compute_row_block);
    // 参考（必做 3 的目标）：
    //   std::for_each(std::execution::par, row_blocks.begin(), row_blocks.end(),
    //                 compute_row_block);
}

// 逐元素校验两个矩阵是否在容差内一致；返回最大绝对差。
static double matrix_max_abs_diff(const float* X, const float* Y, int N) {
    double maxd = 0.0;
    for (int i = 0; i < N * N; ++i)
        maxd = std::max(maxd, std::abs(static_cast<double>(X[i]) - static_cast<double>(Y[i])));
    return maxd;
}

// 打印一行 GEMM 表格：版本名 | 耗时 | 加速比 | GFLOPS | 校验。
// flops = 2 * N^3（每个输出 N 次乘 + N 次加，共 N^2 个输出），口径三版统一。
static void print_gemm_row(const std::string& name, double ms, double base_ms,
                           int N, const std::string& verdict) {
    const double flops = 2.0 * static_cast<double>(N) * N * N;
    const double gflops = flops / (ms / 1000.0) / 1e9;
    const double speedup = base_ms / ms;
    std::ostringstream oss;
    oss << "  " << std::left << std::setw(12) << name
        << "  " << fmt(ms, 10, 2)
        << "  " << fmt(speedup, 8, 2) << "x"
        << "  " << fmt(gflops, 8, 3)
        << "    " << verdict;
    cs::println(oss.str());
}

void run_gemm(int N, int BS, int reps) {
    cs::println("");
    cs::println("==== GEMM (N=" + std::to_string(N) + ", BS=" + std::to_string(BS) + ") ====");
    cs::println("  版本          耗时(ms)     加速比     GFLOPS    校验");

    // 固定种子生成 A、B，保证可复现。
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> A(static_cast<std::size_t>(N) * N);
    std::vector<float> B(static_cast<std::size_t>(N) * N);
    for (auto& x : A) x = dist(rng);
    for (auto& x : B) x = dist(rng);

    std::vector<float> C0(static_cast<std::size_t>(N) * N);  // 基线结果
    std::vector<float> C1(static_cast<std::size_t>(N) * N);
    std::vector<float> C2(static_cast<std::size_t>(N) * N);

    // 预热（不计时）：让线程池/页缓存就绪。
    gemm_naive(A.data(), B.data(), C0.data(), N);

    // v0 基线计时。把一个结果喂给 sink 防优化。
    const double t0 = bench_min_ms([&] {
        gemm_naive(A.data(), B.data(), C0.data(), N);
        g_sink += C0[static_cast<std::size_t>(N) * N / 2];
    }, reps);
    print_gemm_row("v0 naive", t0, t0, N, "基线");

    // v1 分块：先校验再报耗时。
    gemm_tiled(A.data(), B.data(), C1.data(), N, BS); // 预热兼校验数据
    const double d1 = matrix_max_abs_diff(C0.data(), C1.data(), N);
    const std::string v1 = (d1 < 1e-2) ? "OK" : ("FAIL(" + fmt(d1, 0, 4) + ")");
    const double t1 = bench_min_ms([&] {
        gemm_tiled(A.data(), B.data(), C1.data(), N, BS);
        g_sink += C1[static_cast<std::size_t>(N) * N / 2];
    }, reps);
    print_gemm_row("v1 tiled", t1, t0, N, v1);

    // v2 并行分块：先校验再报耗时。
    gemm_par(A.data(), B.data(), C2.data(), N, BS);
    const double d2 = matrix_max_abs_diff(C0.data(), C2.data(), N);
    const std::string v2 = (d2 < 1e-2) ? "OK" : ("FAIL(" + fmt(d2, 0, 4) + ")");
    const double t2 = bench_min_ms([&] {
        gemm_par(A.data(), B.data(), C2.data(), N, BS);
        g_sink += C2[static_cast<std::size_t>(N) * N / 2];
    }, reps);
    print_gemm_row("v2 par", t2, t0, N, v2);
}

// =====================================================================
// 第二部分：大规模并行归约（对照 1）
//   标准版用 std::transform_reduce(par)；手写版用每线程局部累加 +
//   alignas 防伪共享。两者求同一个数组的和。
// =====================================================================

// 标准版：并行归约求和。累加器用 double 以减小 float 求和的累积误差。
double reduce_par_std(const float* data, std::size_t n) {
    // TODO [必做 4]: 用 std::transform_reduce 并行求和。
    //   做法：std::transform_reduce(std::execution::par, data, data+n, 0.0,
    //          std::plus<>{}, [](float x){ return (double)x; });
    //   要点：init=0.0 是 double；二元归约 std::plus 满足结合律（并行归约前提）。
    //   完成后遮住参考实现重写。
    //
    //   占位实现（串行 accumulate，保证可编译运行且结果正确）：
    double s = 0.0;
    for (std::size_t i = 0; i < n; ++i) s += static_cast<double>(data[i]);
    return s;
    // 参考（必做 4 的目标）：
    //   return std::transform_reduce(std::execution::par, data, data + n, 0.0,
    //              std::plus<>{}, [](float x) { return static_cast<double>(x); });
}

// 手写版：把数组分块给 T 个线程，每线程把局部和写进各自独立缓存行的槽位，
// 主线程最后把所有槽位相加。【缓存行对齐】是关键——否则 T 个累加器紧挨着，
// 各线程写各自的和会反复让彼此的缓存行失效（伪共享），手写版反而比标准版慢。
double reduce_par_manual(const float* data, std::size_t n) {
    // 每线程一个局部累加器，alignas(kCacheLine) 保证各占一条独立缓存行（避免伪共享）。
    struct alignas(kCacheLine) Slot {
        double sum = 0.0;
    };

    unsigned T = std::thread::hardware_concurrency();
    if (T == 0) T = 4;
    if (static_cast<std::size_t>(T) > n) T = static_cast<unsigned>(std::max<std::size_t>(1, n));

    std::vector<Slot> slots(T);

    // 每个线程负责区间 [lo, hi) 的局部求和。
    auto worker = [&](unsigned t) {
        const std::size_t chunk = (n + T - 1) / T;
        const std::size_t lo = static_cast<std::size_t>(t) * chunk;
        const std::size_t hi = std::min(lo + chunk, n);
        double local = 0.0;
        for (std::size_t i = lo; i < hi; ++i) local += static_cast<double>(data[i]);
        slots[t].sum = local; // 每线程只写自己的槽位（独立缓存行 → 无伪共享）
    };

    // TODO [必做 5]: 用 T 个 std::thread 并行执行 worker，再 join。
    //   要点：(1) 每线程把局部和写进【自己】的 slots[t]（独立缓存行 → 无伪共享）；
    //         (2) join 后主线程把所有 slots[t].sum 相加得到总和；
    //         (3) 试着把 Slot 的 alignas 去掉，重测对比——亲手复现伪共享导致的变慢。
    //   完成后遮住参考实现重写。
    //
    //   占位实现（串行依次跑每个 worker，保证可编译运行且结果正确）：
    for (unsigned t = 0; t < T; ++t) worker(t);
    // 参考（必做 5 的目标）：
    //   std::vector<std::thread> pool;
    //   for (unsigned t = 0; t < T; ++t) pool.emplace_back(worker, t);
    //   for (auto& th : pool) th.join();

    double total = 0.0;
    for (unsigned t = 0; t < T; ++t) total += slots[t].sum;
    return total;
}

void run_reduce(std::size_t n, int reps) {
    cs::println("");
    cs::println("==== 并行归约 求和 (n=" + std::to_string(n) + ") ====");
    cs::println("  版本              耗时(ms)     加速比     结果         校验");

    std::mt19937 rng(67890);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> data(n);
    for (auto& x : data) x = dist(rng);

    // 串行基线（也作为校验基准）。
    auto serial_sum = [&]() {
        double s = 0.0;
        for (std::size_t i = 0; i < n; ++i) s += static_cast<double>(data[i]);
        return s;
    };
    const double ref = serial_sum();
    const double t_serial = bench_min_ms([&] { g_sink += serial_sum(); }, reps);

    // 标准并行版。
    double r_std = 0.0;
    const double t_std = bench_min_ms([&] { r_std = reduce_par_std(data.data(), n); g_sink += r_std; }, reps);
    // 手写并行版。
    double r_man = 0.0;
    const double t_man = bench_min_ms([&] { r_man = reduce_par_manual(data.data(), n); g_sink += r_man; }, reps);

    // 浮点容差：并行改变了加法结合顺序，结果与串行可能有末位差异（这是结合律，非 bug）。
    const double tol = std::abs(ref) * 1e-6 + 1e-3;
    auto verdict = [&](double v) {
        const double d = std::abs(v - ref);
        return (d <= tol) ? std::string("OK") : ("FAIL(" + fmt(d, 0, 6) + ")");
    };

    auto row = [&](const std::string& name, double ms, double val, const std::string& v) {
        std::ostringstream oss;
        oss << "  " << std::left << std::setw(16) << name
            << "  " << fmt(ms, 10, 3)
            << "  " << fmt(t_serial / ms, 8, 2) << "x"
            << "  " << fmt(val, 12, 2)
            << "  " << v;
        cs::println(oss.str());
    };
    row("serial",         t_serial, ref,   "基线");
    row("par_std",        t_std,    r_std, verdict(r_std));
    row("par_manual(对齐)", t_man,    r_man, verdict(r_man));
}

// =====================================================================
// 第三部分：并行排序（对照 2）
//   std::sort vs std::sort(par)。固定接口，原地排序。
// =====================================================================

void sort_seq(std::vector<int>& v) {
    // 串行排序（基线）。
    std::sort(v.begin(), v.end());
}

void sort_par(std::vector<int>& v) {
    // TODO [必做 6]: 用并行排序替换下面的串行占位。
    //   做法：std::sort(std::execution::par, v.begin(), v.end());
    //   注意：小数组并行可能更慢（线程开销 > 计算量），这正是要观察的趋势。
    //   完成后遮住参考实现重写。
    //
    //   占位实现（串行 sort，保证可编译运行且结果正确；初始加速比≈1.00x，符合预期）：
    std::sort(v.begin(), v.end());
    // 参考（必做 6 的目标）：
    //   std::sort(std::execution::par, v.begin(), v.end());
}

void run_sort(std::size_t n, int reps) {
    cs::println("");
    cs::println("==== 并行排序 (n=" + std::to_string(n) + ") ====");
    cs::println("  版本        耗时(ms)     加速比     校验");

    std::mt19937 rng(2468);
    std::uniform_int_distribution<int> dist(0, 1'000'000'000);
    std::vector<int> base(n);
    for (auto& x : base) x = dist(rng);

    // 每次计时前都要重置成乱序（否则第二次排的是已排好的，计时失真）。
    std::vector<int> work;
    auto seq_once = [&] { work = base; sort_seq(work); g_sink += work.empty() ? 0 : work[n / 2]; };
    auto par_once = [&] { work = base; sort_par(work); g_sink += work.empty() ? 0 : work[n / 2]; };

    // 校验：两种排序结果都必须等于“标准库排好的真值”。
    std::vector<int> truth = base;
    std::sort(truth.begin(), truth.end());
    work = base; sort_seq(work);
    const bool ok_seq = (work == truth);
    work = base; sort_par(work);
    const bool ok_par = (work == truth);

    const double t_seq = bench_min_ms(seq_once, reps);
    const double t_par = bench_min_ms(par_once, reps);

    auto row = [&](const std::string& name, double ms, double base_ms, bool ok) {
        std::ostringstream oss;
        oss << "  " << std::left << std::setw(10) << name
            << "  " << fmt(ms, 10, 2)
            << "  " << fmt(base_ms / ms, 8, 2) << "x"
            << "    " << (ok ? "OK" : "FAIL");
        cs::println(oss.str());
    };
    row("seq", t_seq, t_seq, ok_seq);
    row("par", t_par, t_seq, ok_par);
}

// =====================================================================
// 测试驱动 main()
// =====================================================================
int main() {
    cs::println("==== Capstone3_parallel_compute：并行计算项目（GEMM / 归约 / 排序）====");
    cs::println("  说明：v0/v1 已给真实实现（朴素+分块）；v2 并行、归约多线程、并行排序的占位仍为串行，");
    cs::println("  故这些版本初始加速比≈1.00x（符合预期）。按 // TODO [必做 N] 换上 par/多线程后才会拉开。");
    cs::println("  方法学：预热 + 多跑取最小值 + volatile sink 防优化；务必在 Release/优化下观察趋势。");
    cs::logf("  hardware_concurrency = ", std::thread::hardware_concurrency(),
             "，缓存行常量 kCacheLine = ", kCacheLine, " 字节");

    // 主线：矩阵乘。N=512 适中（256KB/矩阵），BS=64 是常见起点。
    run_gemm(/*N=*/512, /*BS=*/64, /*reps=*/3);

    // 对照 1：大规模并行归约。n=2^24 ≈ 1678 万个 float（约 64MB）。
    run_reduce(/*n=*/1u << 24, /*reps=*/5);

    // 对照 2：并行排序。两个规模对比，体会“并行收益与规模强相关”。
    run_sort(/*n=*/1u << 18, /*reps=*/5);   // 小规模：并行可能更慢
    run_sort(/*n=*/1u << 23, /*reps=*/5);   // 大规模：并行才开始划算

    cs::println("");
    cs::println("==== 全部任务完成。校验列若有 FAIL，说明对应版本实现有误，请先修对再看耗时。 ====");

    // TODO [进阶 1]: 显式 SIMD 向量化（指回模块 K）。
    //   本题主体只靠【编译器自动向量化 + SIMD-friendly 循环】（v1 内层的
    //   crow[j] += a*brow[j] 就是为自动向量化写的）。进阶方向：用 std::simd
    //   （C++26，模块 K）或 xsimd 把 GEMM 内核 / 归约【显式】向量化（一条指令
    //   算 8 个 float），与自动向量化对比 GFLOPS，体会“多拿的收益 vs 多付的代价”。
    //   实现与对照见：Concurrency_Study/14-模块K-数据并行与std-simd.md，
    //   以及练习 K-1 / K-2（SIMD 点积）。

    // 防止 g_sink 被认为完全无用（极端优化下）：把它读出来用一下。
    if (g_sink == 1234.56789) cs::println("(sink 校验线，永不触发)");
    return 0;
}
