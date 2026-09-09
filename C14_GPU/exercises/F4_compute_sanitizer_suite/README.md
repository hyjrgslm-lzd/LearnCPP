# 练习 F4：Compute Sanitizer 与 bug 检测

## 目标

`[F4-T01]` (main.cu:2) 练习 F4：Compute Sanitizer 与 bug 检测。
`[F4-T02]` (main.cu:3) 学习目标：4 个故意有 bug 的 kernel，分别触发四类 sanitizer 报告（memcheck 越界访问 / racecheck shared memory 数据竞争 / synccheck divergent `__syncthreads` / initcheck 读取未初始化 shared memory）；用 `--bug=oob/race/sync/uninit` 参数选择运行哪个 bug。
`[F4-T03]` (main.cu:11) 编译命令。
`[F4-T04]` (main.cu:12) 检测命令（分别运行）。
`[F4-T05]` (main.cu:17) 正常运行（无参数）：全部触发。

学会用 compute-sanitizer 工具套件的四个检测器（memcheck、racecheck、synccheck、initcheck），抓住那些 Nsight Compute 看不到、运行时才爆的 bug。通过构造故意错误的 kernel，理解每个检测器的工作原理。

## 前置理解

- 你知道共享内存的 bank conflict（虽然不是 bug，但 Nsight 会报）。
- 你理解"data race"（数据竞争）的概念（多线程写同一块内存）。
- 你知道 `__syncthreads` 的作用，以及不同步的后果。

## 编译

```bash
cmake --build build --target F4_compute_sanitizer_suite
```

> **重要**：为获得最准确的检测结果，建议用 Debug 配置编译，保留调试符号：
> ```bash
> cmake --build build --config Debug --target F4_compute_sanitizer_suite
> ```

## 运行与 Sanitizer 命令

### 分别检测各 bug

```bash
# BUG #1：越界写（out-of-bounds write）
compute-sanitizer --tool memcheck   ./F4_compute_sanitizer_suite --bug=oob

# BUG #2：shared memory 数据竞争（non-atomic RMW）
compute-sanitizer --tool racecheck  ./F4_compute_sanitizer_suite --bug=race

# BUG #3：divergent __syncthreads
compute-sanitizer --tool synccheck  ./F4_compute_sanitizer_suite --bug=sync

# BUG #4：读取未初始化 shared memory
compute-sanitizer --tool initcheck  ./F4_compute_sanitizer_suite --bug=uninit
```

### 全部同时运行（无参数）

```bash
compute-sanitizer ./F4_compute_sanitizer_suite
```

### 重定向输出到文件

```bash
compute-sanitizer --tool memcheck ./F4_compute_sanitizer_suite --bug=oob 2>&1 | tee memcheck_report.txt
```

### 增加日志详细程度

```bash
compute-sanitizer --tool racecheck --log-level info ./F4_compute_sanitizer_suite --bug=race
```

## 必做任务

`[F4-T06]` (main.cu:24) 常量定义。
`[F4-T07]` (main.cu:25) 数据规模（小，让 sanitizer 快速报告）。
`[F4-T08]` (main.cu:27) 越界偏移量（故意超出分配范围）。
`[F4-T09]` (main.cu:30) INTENTIONAL BUG #1：越界写（memcheck 检测）；每个线程写 `global_mem[tid + OOB_EXTRA]`，其中 `tid < N` 但分配大小只有 `N*sizeof(float)`，`OOB_EXTRA` 超出分配末尾。
`[F4-T10]` (main.cu:43) INTENTIONAL BUG #1 — out-of-bounds write, triggers memcheck。
`[F4-T11]` (main.cu:44) 故意越界：`tid` 在 `[0, n)` 范围内，但写地址加上 `OOB_EXTRA` 超出分配末尾。
`[F4-T12]` (main.cu:51) INTENTIONAL BUG #2：shared memory 数据竞争（racecheck 检测）；两个 warp 写同一个 `__shared__` 地址，无任何同步。
`[F4-T13]` (main.cu:64) INTENTIONAL BUG #2 — shared mem counter without atomics, triggers racecheck。
`[F4-T14]` (main.cu:65) 无初始化、无同步。
`[F4-T15]` (main.cu:68) BUG：所有线程并发写 `shared_counter`，无原子操作，产生数据竞争。
`[F4-T16]` (main.cu:80) INTENTIONAL BUG #3：divergent `__syncthreads`（synccheck 检测）；`threadIdx.x < 16` 的线程进入 if 分支并调用 `__syncthreads()`，其余线程跳过，导致 warp 内只有部分线程参与同步。
`[F4-T17]` (main.cu:91) INTENTIONAL BUG #3 — `__syncthreads` in divergent branch, triggers synccheck。
`[F4-T18]` (main.cu:97) BUG：只有 `threadIdx.x < 16` 的线程调用 `__syncthreads`，其余跳过 — divergent。
`[F4-T19]` (main.cu:106) INTENTIONAL BUG #4：读取未初始化 shared memory（initcheck 检测）；只有 `threadIdx.x == 0` 初始化了 `smem[0]`，其余位置从未赋值，所有线程都读 `smem[threadIdx.x]`。
`[F4-T20]` (main.cu:118) INTENTIONAL BUG #4 — reads uninitialized `__shared__`, triggers initcheck。
`[F4-T21]` (main.cu:122) BUG：只初始化 `smem[0]`，其余位置读取时未赋值。
`[F4-T22]` (main.cu:124) 只有 index 0 被初始化。
`[F4-T23]` (main.cu:126) 此处故意不加 `__syncthreads` 以确保 BUG #4 可被 initcheck 检测（加了 `__syncthreads` 不影响 initcheck，但 `smem[1..127]` 仍未初始化）。
`[F4-T35]` (main.cu:248) TODO [必做-1] 对每个 bug，记录 sanitizer 报告的关键字段（错误类型 / block-thread ID / 内存地址）。
`[F4-T36]` (main.cu:257) TODO [必做-2] 修复每个 bug，重新运行对应 sanitizer，验证错误消失。
`[F4-T37]` (main.cu:263) TODO [必做-3] 计算各 bug 版本与修复版本的性能差距（sanitizer 本身开销 vs 修复收益）。

### BUG #1：`kernel_oob` — 越界写（memcheck）

```cuda
// 分配了 N 个元素，但写 global_mem[tid + OOB_EXTRA]，超出末尾 OOB_EXTRA 个位置
global_mem[tid + OOB_EXTRA] = static_cast<float>(tid);  // INTENTIONAL BUG #1
```

**预期 memcheck 报告**：
```
Invalid __global__ write of size 4
  at 0x... in kernel_oob
  Address 0x... is out of bounds
  Leaked 1 error(s)
```

**修复方法**：将 `tid + OOB_EXTRA` 改为 `tid`，确保写地址在分配范围内。

### BUG #2：`kernel_race` — shared memory 数据竞争（racecheck）

```cuda
// 所有线程并发 RMW shared_counter，无原子操作
shared_counter = shared_counter + 1;  // INTENTIONAL BUG #2
```

**预期 racecheck 报告**：
```
Shared memory race hazard: write/read
  between thread (x1, 0, 0) and thread (x2, 0, 0)
  at offset 0x0 in __shared__ memory
```

**修复方法**：改为 `atomicAdd(&shared_counter, 1)`，消除并发 RMW 竞争。

### BUG #3：`kernel_div_sync` — divergent `__syncthreads`（synccheck）

```cuda
// 只有 threadIdx.x < 16 的线程调用 __syncthreads，其余跳过
if (threadIdx.x < 16) {
    __syncthreads();  // INTENTIONAL BUG #3
    out[tid] = smem[threadIdx.x] * 2.0f;
}
```

**预期 synccheck 报告**：
```
__syncthreads() in divergent code
  at block (x, 0, 0), CTA dimension (128, 1, 1)
  not all threads participate
```

**修复方法**：将 `__syncthreads()` 移出 `if` 分支，确保 block 内所有线程都执行同步。

### BUG #4：`kernel_uninit` — 读取未初始化 shared memory（initcheck）

```cuda
// 只有 threadIdx.x == 0 初始化了 smem[0]，其余 smem[1..127] 未初始化
if (threadIdx.x == 0) smem[0] = 42.0f;
out[tid] = smem[threadIdx.x];  // INTENTIONAL BUG #4：smem[1..127] 未初始化
```

**预期 initcheck 报告**：
```
Uninitialized __shared__ memory read of size 4
  at thread (1, 0, 0) in block (0, 0, 0)
  at 0x... in kernel_uninit
```

**修复方法**：在读取前完整初始化 `smem`（例如 `smem[threadIdx.x] = 0.0f`），再加 `__syncthreads()`。

## 四类 sanitizer 对照表

| Bug 类型 | Sanitizer 工具 | 检测原理 | 性能开销 |
|---|---|---|---|
| 越界访问 / UAF（use-after-free，释放后使用）| `memcheck` | 拦截所有全局内存访问，检查地址合法性 | 10-20x |
| Shared mem 数据竞争 | `racecheck` | SM（流式多处理器）级追踪所有 shared mem 读写顺序 | 20-100x |
| Divergent `__syncthreads` | `synccheck` | 追踪每个 warp 的同步原语调用路径 | 5-10x |
| 未初始化读取 | `initcheck` | 为每个 byte 维护"已初始化"位 | 10-30x |

## 观察点

`[F4-T24]` (main.cu:139) `main` 入口。
`[F4-T25]` (main.cu:147) 解析 `--bug=xxx` 参数。
`[F4-T26]` (main.cu:165) 分配内存。
`[F4-T27]` (main.cu:167) 故意只分配 N 个元素，`kernel_oob` 会越界写 `OOB_EXTRA` 位置。
`[F4-T28]` (main.cu:172) BUG #1 会越界。
`[F4-T29]` (main.cu:173) BUG #2 结果。
`[F4-T30]` (main.cu:174) BUG #3 / #4 输出。
`[F4-T31]` (main.cu:182) BUG #1：`kernel_oob`（memcheck）。
`[F4-T32]` (main.cu:198) BUG #2：`kernel_race`（racecheck）。
`[F4-T33]` (main.cu:215) BUG #3：`kernel_div_sync`（synccheck）。
`[F4-T34]` (main.cu:231) BUG #4：`kernel_uninit`（initcheck）。

- memcheck 检测越界访问、UAF、memory leak 等；是最常见的工具。
- racecheck 检测 shared memory 中的数据竞争；需要开启 SM level 追踪。
- synccheck 检测 `__syncthreads` 和其它同步原语的发散；可以防止死锁。
- initcheck 检测未初始化的读取；对数值稳定性很关键。
- compute-sanitizer 的开销很大（kernel 变慢 10-100 倍），所以通常只在调试时用。

## 常见坑

1. 某些检测器需要特定的 Compute Capability（计算能力，例如 racecheck 需要 CC 7.0+）；在不支持的硬件上会失败。
2. compute-sanitizer 输出信息量很大，容易遗漏关键错误；需要仔细阅读报告。
3. 关闭了某些优化选项或改变了编译参数后，某些 bug 可能无法检测。
4. 误以为检测器的缺席 = bug 不存在；某些隐蔽的 race 可能逃脱检测。
5. 在 memcheck 模式下，某些内存操作可能被标记为"maybe error"而非"definite error"；需要判断是否真的是 bug。

## 进阶任务

`[F4-T38]` (main.cu:266) TODO [进阶-1] 构造一个包含多个隐蔽 bug 的 kernel，用 `--tool all` 全部检测。
`[F4-T39]` (main.cu:267) TODO [进阶-2] 用 `--log-level info` 获取更详细的 sanitizer 日志。
`[F4-T40]` (main.cu:268) TODO [进阶-3] 在正确的 kernel 上运行所有四个 sanitizer，验证无误报。

- 构造一个包含多个隐蔽 bug 的 kernel，运行 `compute-sanitizer --tool all` 看是否能全部检测。
- 尝试用 `--debug full` 或其它参数获得更详细的调试信息（可能需要 debug symbols）。
- 在一个正确的 kernel 上运行所有四个检测器，观察是否误报（应该没有）。

## 验收点

`[F4-T41]` (main.cu:271) 清理。
`[F4-T42]` (main.cu:280) 完成提示。

- 四个 kernel 分别编译通过。
- 每个检测器都成功检测到对应的 bug。
- 修复后，检测器不再报错。
- 记录了每类 bug 的症状和修复方法。

## 复盘问题

1. memcheck 和 racecheck 各检测什么类型的 bug？它们能同时运行吗？
2. synccheck 如何检测 divergent `__syncthreads`？
3. initcheck 如何知道一块 shared memory 没被初始化？
4. compute-sanitizer 的性能开销为什么这么大？

## 对应官方参考

- compute-sanitizer Documentation：https://docs.nvidia.com/cuda/compute-sanitizer/

## 输出对照（printf / std::puts 原文）

- `[F4-T31]` (main.cu:185) 原文：`--- BUG #1: kernel_oob（compute-sanitizer --tool memcheck 检测）---` -> 现：`--- BUG #1: kernel_oob (compute-sanitizer --tool memcheck) ---`；`预期: ...` -> `expected: ...`；`无 sanitizer 时不崩溃（写入邻近内存），但 memcheck 会报告越界` -> `no-sanitizer run: does not crash (writes neighboring memory) but memcheck flags it`。
- `[F4-T32]` (main.cu:201) 原文：`--- BUG #2: kernel_race（compute-sanitizer --tool racecheck 检测）---` -> 现：`--- BUG #2: kernel_race (compute-sanitizer --tool racecheck) ---`；`无 sanitizer 时结果不确定（数据竞争），racecheck 会报告 hazard` -> `no-sanitizer run: result is nondeterministic (data race); racecheck reports hazard`。
- `[F4-T33]` (main.cu:218) 原文：`--- BUG #3: kernel_div_sync（compute-sanitizer --tool synccheck 检测）---` -> 现：`--- BUG #3: kernel_div_sync (compute-sanitizer --tool synccheck) ---`；`无 sanitizer 时可能产生未定义行为，synccheck 会报告 divergent sync` -> `no-sanitizer run: undefined behavior; synccheck reports divergent sync`。
- `[F4-T34]` (main.cu:234) 原文：`--- BUG #4: kernel_uninit（compute-sanitizer --tool initcheck 检测）---` -> 现：`--- BUG #4: kernel_uninit (compute-sanitizer --tool initcheck) ---`；`无 sanitizer 时读到垃圾值，initcheck 会报告未初始化读` -> `no-sanitizer run: reads garbage; initcheck reports uninitialized read`。
- 启动信息 `启动:` -> `launch:`。
- `[F4-T42]` (main.cu:283) 原文：`[F4] 完成。上述 kernel 包含故意 bug，请用 compute-sanitizer 对应工具检测。` -> 现：`[F4] done. The kernels above contain intentional bugs; run compute-sanitizer with the matching tool.`
