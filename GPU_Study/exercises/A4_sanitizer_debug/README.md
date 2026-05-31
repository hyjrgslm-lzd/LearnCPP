# 练习 A4：sanitizer_debug

## 目标

`[A4-T01]` (main.cu:2) 练习 A4：sanitizer_debug。
`[A4-T02]` (main.cu:3) 用 `compute-sanitizer` 定位内存访问错误（out-of-bounds），
`[A4-T03]` (main.cu:4) 以及数据竞争（race condition）。

用 `compute-sanitizer` 定位内存访问错误（out-of-bounds）和数据竞争（race condition）。学会用 Nsight VSE 在 kernel 中设置断点调试。

## 前置理解

- 完成了 A1/A2。
- 你知道内存越界和数据竞争是什么。
- 接受 GPU 编程中，不能像 CPU 那样随意用 debugger（但 Nsight VSE 提供了基础支持）。

## 必做任务

`[A4-T05]` (main.cu:6) 必做工作流。
`[A4-T06]` (main.cu:7) memcheck 工作流提示。
`[A4-T07]` (main.cu:8) `compute-sanitizer memcheck A4_sanitizer_debug.exe`。
`[A4-T08]` (main.cu:9) racecheck 工作流提示。
`[A4-T09]` (main.cu:10) `compute-sanitizer --tool racecheck A4_sanitizer_debug.exe`。
`[A4-T10]` (main.cu:11) synccheck 工作流提示。
`[A4-T11]` (main.cu:12) `compute-sanitizer --tool synccheck A4_sanitizer_debug.exe`。
`[A4-T13]` (main.cu:14) 注意：本文件故意包含 3 个有 bug 的 kernel。
`[A4-T14]` (main.cu:15) 练习目标是用 sanitizer 找到它们，再修复。
`[A4-T15]` (main.cu:16) 修复后 sanitizer 应报告 "no errors detected"。
`[A4-T17]` (main.cu:27) Bug Kernel 1：故意越界写。
`[A4-T18]` (main.cu:29) TODO [必做] 步骤 1：找到 kernel 里的越界错误。提示：注意 `if` 条件里的边界。
`[A4-T19]` (main.cu:33) BUG：使用 `n+1` 会让最后一个线程越界写 `data[n]`。
`[A4-T20]` (main.cu:41) TODO [必做] 步骤 4：修复版本，把 `n+1` 改为 `n`。
`[A4-T22]` (main.cu:51) Bug Kernel 2：shared memory 数据竞争。
`[A4-T23]` (main.cu:53) TODO [必做] 步骤 5：找到 kernel 里的数据竞争。提示：多个线程写同一个 global 地址，且无原子操作。
`[A4-T24]` (main.cu:58) BUG：两个线程都写 `flag[0]`，产生数据竞争。
`[A4-T25]` (main.cu:60) `flag[0]++` 非原子：读-改-写，竞争！
`[A4-T26]` (main.cu:65) TODO [必做] 步骤 7：修复版本，用 `atomicAdd`。
`[A4-T28]` (main.cu:76) Bug Kernel 3：缺少 `__syncthreads`（sync 错误）。
`[A4-T29]` (main.cu:78) TODO [必做]（进阶）步骤：在分支内调用 `__syncthreads`，只有部分线程到达，会触发 synccheck 报错。
`[A4-T41]` (main.cu:142) TODO [必做] 步骤 2：启动时恰好有 1001 个线程（触发越界）。
`[A4-T44]` (main.cu:154) TODO [必做] 步骤 3：用 `compute-sanitizer memcheck` 运行并观察输出。
`[A4-T48]` (main.cu:181) TODO [必做] 步骤 6：用 `compute-sanitizer --tool racecheck` 运行。

1. `main.cu` 中已有一个 kernel `buggy_oob_kernel`，故意造了一个 out-of-bounds 访问：

```cpp
// TODO [必做] __global__ void buggy_oob_kernel(int *data, int n) {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     if (idx < n + 1) {  // 注意：n+1 会越界！
//         data[idx] = idx;
//     }
// }
```

2. 在 `main()` 里已分配大小 1000 的数组，启动 kernel 使得超过 1000 个线程运行，触发越界。
3. 用 `compute-sanitizer memcheck ./a4_sanitizer_debug.exe` 运行程序，观察输出（会打印越界的线程编号和地址）。

```
// TODO [必做] 在 build 目录执行：compute-sanitizer memcheck ./A4_sanitizer_debug.exe
```

4. 修复代码：找到 `main.cu` 中的 `fixed_oob_kernel`（被 `#if 0` 注释），启用它，改成 `if (idx < n)`，再用 memcheck 验证没有错误。
5. 文件中还有一个 kernel `buggy_race_kernel`，造成数据竞争：

```cpp
// TODO [必做] 所有线程都写同一个地址！
// flag[0]++;  // 非原子：读-改-写，竞争！
```

6. 用 `compute-sanitizer --tool racecheck ./A4_sanitizer_debug.exe` 运行，观察输出（会报告数据竞争）。
7. 修复代码：用 `atomicAdd` 替换，再用 racecheck 验证。
8. （可选）在 Visual Studio 2026 中用 Nsight VSE，设置 kernel 断点，观察线程状态（需要 `-g` 编译标志）。

## 进阶任务

`[A4-T32]` (main.cu:101) TODO [进阶] 修复版本：把 `__syncthreads()` 移到分支外。
`[A4-T54]` (main.cu:206) TODO [进阶] 把 `#if 0` 改为 `#if 1`，在 `compute-sanitizer synccheck` 下运行。

- 在 `buggy_race_kernel` 中，不用 atomic，而是用 shared memory + `__syncthreads()` 来安全累加，再用 racecheck 验证没有数据竞争。
- 用 `compute-sanitizer --tool synccheck` 检查同步错误（缺少 `__syncthreads` 导致的死锁）。
- 修改 kernel 使得只有某些线程会竞争，观察 sanitizer 如何定位具体的线程对。

## 验收点

`[A4-T04]` (main.cu:5) （空行注释）。
`[A4-T12]` (main.cu:13) （空行注释）。
`[A4-T16]` (main.cu:17) （文件总目标说明结束）。
`[A4-T21]` (main.cu:44) 修复版本中正确的边界 `if (idx < n)`。
`[A4-T27]` (main.cu:71) 修复版本：`atomicAdd` 原子操作，无竞争。
`[A4-T30]` (main.cu:88) BUG：`__syncthreads` 在条件分支内，只有部分线程到达。
`[A4-T31]` (main.cu:90) 奇数线程永远不会到达这里 -> 死锁！
`[A4-T33]` (main.cu:107) 修复版本中 `__syncthreads()` 在分支外，所有线程都能到达。
`[A4-T34]` (main.cu:118) `main` 入口。
`[A4-T40]` (main.cu:135) Test 1：out-of-bounds。
`[A4-T42]` (main.cu:144) `ceil(1001/256)*256 = 1024 >= 1001` 个线程运行。
`[A4-T43]` (main.cu:147) 注意：`(N + block_oob.x) / block_oob.x = 5 blocks = 1280 threads > 1001`。
`[A4-T46]` (main.cu:166) Test 2：数据竞争。
`[A4-T47]` (main.cu:175) 32 个线程，其中 thread 0 和 1 会竞争。
`[A4-T50]` (main.cu:194) Test 3：sync 错误。
`[A4-T51]` (main.cu:195) 注意：此 kernel 在 sanitizer 外运行可能挂起/超时。
`[A4-T52]` (main.cu:196) 只在 sanitizer 下运行时才解除 `#if 0`。
`[A4-T53]` (main.cu:197) （注释续）。

- `compute-sanitizer memcheck` 在修复前能检出 out-of-bounds 错误，修复后报告"no errors"。
- `compute-sanitizer racecheck` 在修复前能检出数据竞争，修复后报告"no errors"。
- 没有 compilation error。
- 代码能正确运行（不崩溃，输出正确）。

## 观察点

- `compute-sanitizer memcheck` 相对轻量，能快速定位越界。它会暂停访问越界的线程并报告。
- `compute-sanitizer racecheck` 需要动态追踪，性能开销大，但能定位并发写同一地址的所有线程对。
- Nsight VSE 的 kernel 调试功能有限（不像 CPU debugger 那么灵活），但足以观察线程状态和寄存器值。
- 大多数 GPU 缺陷来自"假设线程之间有序执行"或"忘记同步"，sanitizer 是快速抓住这些的工具。

## 常见坑

1. **compute-sanitizer 找不到可执行文件**：确保路径正确且文件已编译（不能跑 object file）。
2. **memcheck 输出太多，看不清**：用管道过滤前 50 行。
3. **racecheck 误报或漏报**：dynamic analysis 存在假正和假负。memcheck 更可信。
4. **编译时没有 debug 信息**：为了看到源代码行号，编译时加 `--generate-line-info` 或 `-g`。
5. **忘记同步 device 和 host**：某些错误只在 `cudaDeviceSynchronize()` 后才会被 sanitizer 检出。

## 提示

- `atomicAdd(addr, val)` 是原子加法，保证多线程写同一个地址时不会丢失。
- `__syncthreads()` 只同步 block 内的线程，不同 block 的线程无法用它同步。
- sanitizer 的性能开销很大（10-100x），只用来快速抓缺陷，不用来做性能测量。
- 修复方法汇总：
  - OOB：改边界条件 `idx < n`
  - Race：用 `atomicAdd` 或 shared memory reduce
  - Sync：把 `__syncthreads()` 移到分支外

## 复盘问题

1. `compute-sanitizer memcheck` 能检出哪些错误？`racecheck` 呢？
2. 为什么说 racecheck 的开销比 memcheck 大？
3. 修复数据竞争有哪几种办法？各有什么优缺点？
4. Nsight VSE 的 kernel 调试和 CPU 调试有什么根本区别？
5. 如果 sanitizer 没有检出某个缺陷，是不是说代码是对的？

## 对应官方参考

- compute-sanitizer: https://docs.nvidia.com/cuda/compute-sanitizer/
- Nsight Visual Studio Edition: https://docs.nvidia.com/nsight-visual-studio-edition/
- CUDA Runtime API: https://docs.nvidia.com/cuda/cuda-runtime-api/（atomic 函数）

## 输出对照（printf / std::puts 原文）

- `[A4-T35]` (main.cu:124) 原文："  本程序包含 3 个故意引入的 bug kernel。" -> 现："  This program contains 3 deliberately buggy kernels."
- `[A4-T36]` (main.cu:126) 原文："  请用 compute-sanitizer 定位，然后修复。" -> 现："  Use compute-sanitizer to locate them, then fix them."
- `[A4-T37]` (main.cu:128) 原文："  修复方法：将 #if 0 / #endif 包裹的修复版本启用，" -> 现："  Fix recipe: enable the fixed_xxx variant inside the"
- `[A4-T38]` (main.cu:130) 原文："            并替换 buggy_xxx 的调用为 fixed_xxx。" -> 现："              #if 0 / #endif block, and replace the"
- `[A4-T39]` (main.cu:132) 原文：（无对应中文行；此为续行的英文 "buggy_xxx call sites with fixed_xxx."） -> 现："              buggy_xxx call sites with fixed_xxx."
- `[A4-T45]` (main.cu:160) 原文："[A4] OOB kernel 执行完毕（sanitizer 应报告越界）" -> 现："[A4] OOB kernel finished (sanitizer should report OOB)"
- `[A4-T49]` (main.cu:188) 原文："[A4] Race kernel 完毕: flag=%d (期望 2000, 但竞争导致结果不确定)" -> 现："[A4] Race kernel done: flag=%d (expected 2000, but race makes the result undefined)"
- `[A4-T55]` (main.cu:218) 原文："[A4] sync kernel 完毕（compute-sanitizer synccheck 应报告错误）" -> 现："[A4] sync kernel done (compute-sanitizer synccheck should report)"
- `[A4-T56]` (main.cu:225) 原文："  TODO [必做] 完成后：" -> 现："  TODO [REQUIRED] when finished:"
- `[A4-T57]` (main.cu:227) 原文："    1. 取消注释修复版本（fixed_xxx kernel）" -> 现："    1. Uncomment the fixed_xxx kernel variants"
- `[A4-T58]` (main.cu:229) 原文："    2. 将 main() 中的 buggy_xxx 替换为 fixed_xxx" -> 现："    2. Replace buggy_xxx with fixed_xxx in main()"
- `[A4-T59]` (main.cu:231) 原文："    3. 重新用 compute-sanitizer 运行，确认 no errors" -> 现："    3. Re-run compute-sanitizer and confirm 'no errors'"
