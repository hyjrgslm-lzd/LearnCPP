# 模块 D · Warp 原语与占用率

## 模块目标

掌握 warp 级的计算原语（shuffle、vote、ballot）和内存优化（`__ldg`、read-only cache），理解占用率与实际性能的微妙关系，学会用 `__launch_bounds__` 调控寄存器压力和 occupancy 的权衡。本模块的目标不是"占用率越高越好"，而是"理解占用率的作用与限制"。

## 前置知识

- 完成模块 C（线程层级与同步原语）
- 理解 block/warp/lane 映射
- 理解 `__syncthreads` 和 shared memory 的场景

## 模块完成标准

- 掌握五大 warp shuffle 函数（broadcast、shift、butterfly），能用 shuffle 写 warp-local reduce 和 scan
- 掌握 vote 和 ballot 函数（`__any_sync`、`__all_sync`、`__ballot_sync`），能用 ballot 做 stream compaction
- 理解 `__ldg` 和 read-only cache 的工作原理，知道什么时候有效
- 能用 `cudaOccupancyMaxPotentialBlockSize` 和 `__launch_bounds__` 调节 blockDim、寄存器用量、occupancy 的平衡
- 能用 Nsight Compute 的 roofline 和瓶颈分类指标区分 compute-bound / memory-bound / latency-bound
- 理解 occupancy 高不等于性能好的真实例子

## 硬件与工具链要求

- Compute Capability：sm_80+（warp shuffle / vote / ballot 基础）
- CUDA Toolkit：13.x+
- Nsight Compute：必需（roofline、bottleneck classification、register spill 检测）
- PTX 查看工具：用于验证 `__ldg` 是否生成 `ld.global.nc` 指令

---

## 练习 D1：warp_shuffle_basics

### 目标

学会五大 warp shuffle 函数，理解为什么 warp-local 操作比 shared memory 快，以及如何用 shuffle 实现 warp-32 级的 reduce 和 scan。

### 前置理解

- warp 是 32 个线程的集合，硬件调度的最小单位
- lane ID 是 warp 内的线程编号（0–31）
- shuffle 操作不需要 shared memory，直接从寄存器读数据

### 必做任务

1. // TODO [必做] 用 `__shfl_sync` 实现一个 warp 内的 broadcast：lane 0 的值广播给其他 31 个 lane，验证所有 lane 都读到相同值。
2. // TODO [必做] 用 `__shfl_xor_sync` 实现一个"蝶形"（butterfly exchange）：lane 0 和 1 交换，lane 2 和 3 交换，以此类推。验证交换结果正确。
3. // TODO [必做] 用 `__shfl_down_sync` 实现"移位"：每个 lane 读上游 offset 步的 lane 的值，逐步构建一个 warp 内的 reduce。
4. // TODO [必做] 用上述 shuffle 实现一个 warp-32 的求和 reduce：每轮迭代，offset 指数增大（1, 2, 4, 8, 16），逐步汇合结果到 lane 0。
5. // TODO [必做] 实现一个 warp-level scan（前缀和）：Kogge-Stone 算法，log(32) 轮，每轮用 `__shfl_up_sync` 读前驱。
6. // TODO [必做] 对比 shuffle 版和 shared memory 版本的性能（用 Nsight Compute 或简单计时）。shuffle 应该更快。

### 进阶任务

- 实现 Brent-Kung scan（不同的流水线策略），对比 Kogge-Stone
- 尝试 warp-16 或 warp-8 的 shuffle（用 mask 参数限制参与的 lane），观察部分 warp 参与时的行为

### 验收点

- warp reduce 和 scan 的结果与 CPU 参考实现一致
- 能清晰指出每个 `__shfl_*` 调用的 offset 含义
- Nsight Compute 显示 shuffle 版本的延迟或吞吐优于 smem 版本
- 代码没有寄存器溢出（register spill）

### 观察点

- warp shuffle 利用了"同一 warp 自动前向进度"的特性，无需显式屏障
- shuffle 的 mask 参数用于指定参与的 lane 集合（通常是 `0xffffffff`，表示全 warp）
- 同一个 warp 内的 shuffle 没有内存访问，全在寄存器和片上通路上，所以很快
- warp reduce / scan 是后续 block-level reduce / scan 的基础块

### 常见坑

- shuffle mask 错误（例如 mask 不包含某个 lane，导致该 lane 读到 undefined 值）
- 混淆 `__shfl_up` 和 `__shfl_down` 的方向
- offset 设置不对，导致某些数据被读多次或没被读
- 在非 warp 边界的地方结束 shuffle 操作，导致某些线程看到的数据不一致
- 假设 warp shuffle 可以跨 block，实际只在同一 warp 内有效
- 没有在 Kogge-Stone 或 Brent-Kung 的每一步加上所需的同步（warp 内无需显式 sync，但要理解数据依赖）

### 提示

- warp reduce 标准模式：`for(int offset = 16; offset > 0; offset >>= 1) { value += __shfl_down_sync(0xffffffff, value, offset); }`
- Kogge-Stone scan：`for(int offset = 1; offset < 32; offset <<= 1) { int prev = __shfl_up_sync(0xffffffff, value, offset); if(laneId >= offset) value += prev; }`
- mask 常用值：`0xffffffff`（全 warp）、`0x0000ffff`（低 16 lane）、根据需要构造
- 验证：shuffle 结果应与对应的 smem 版本一致

### 复盘问题

- warp reduce 需要多少轮 shuffle？为什么是这个数？
- Kogge-Stone 和 Brent-Kung 对于 scan 分别有什么优缺点？
- 为什么 warp shuffle 比 shared memory 快？（提示：内存层级）
- 如果只想操作 warp 的低 16 lane，mask 应该怎样设置？

### 对应官方参考

- CUDA C++ Programming Guide Section C.22: "Warp Shuffle Functions"
- CUDA C++ Best Practices: "Warp Shuffle" section
- NVIDIA/cccl CUB library: `warp_reduce` example
- CUDA Samples: `reduction` sample code

---

## 练习 D2：vote_ballot

### 目标

学会 vote (`__any_sync`、`__all_sync`) 和 ballot (`__ballot_sync`) 函数，用 ballot 实现 stream compaction（条件过滤），以及计算 warp-level 的条件统计。

### 前置理解

- 理解 warp 内的 divergence（同一 warp 的线程进入不同分支）
- 理解 ballot 的结果是一个 32-bit 整数，每一 bit 对应一个 lane 的条件结果

### 必做任务

1. // TODO [必做] 实现 `__any_sync`：检查同一 warp 的任意 lane 的条件是否为真。例如，任何 lane 的值 > 100。
2. // TODO [必做] 实现 `__all_sync`：检查同一 warp 的全部 lane 的条件是否全为真。例如，所有 lane 的值都 > 0。
3. // TODO [必做] 用 `__ballot_sync` 构造一个 32-bit 掩码，其中第 i 位表示 lane i 的条件是否为真。
4. // TODO [必做] 用 ballot 实现 stream compaction：输入数组，过滤出满足条件的元素（例如偶数），输出到紧凑数组。用 popcount 计算输出大小。
5. // TODO [必做] 对比 ballot 版本和 shared memory gather 版本的正确性和性能。
6. // TODO [必做] 在 Nsight Compute 中查看两个版本的 PTX，观察 `vote.any` / `ballot` 指令的分布。

### 进阶任务

- 实现一个 warp-level 的直方图：统计满足不同条件的 lane 个数，输出为 vector
- 用 ballot 构造一个"lane 活跃度"的可视化（打印每个 warp 有多少 active lane）

### 验收点

- vote 和 ballot 的结果与预期相符
- stream compaction 的输出完全而无重复
- 性能对比显示 ballot 版本相较 smem gather 不差（或更优）
- Nsight Compute PTX 视图确认生成了 vote / ballot 指令

### 观察点

- `__any_sync` 和 `__all_sync` 是归约操作，返回单一布尔值
- `__ballot_sync` 返回 32-bit 掩码，编码了每个 lane 的条件
- ballot + popcount 可以快速计算满足条件的 lane 个数
- divergence 内的 vote/ballot 结果反映实际的 lane 状态分布

### 常见坑

- vote mask 错误，导致某些 lane 被忽略
- 在 divergent branch 外调用 `__activemask()` 而非明确的 sync mask，导致不准确
- 混淆 `__any_sync` 和 `__all_sync` 的语义（any = OR，all = AND）
- ballot 结果理解错误（例如不知道哪个 bit 对应哪个 lane）
- 没有考虑到 popcount 的成本（实际上很快，但仍不是零代价）
- stream compaction 时忘记原子操作或同步，导致输出数据混乱

### 提示

- vote 函数返回 0（false）或 1（true）
- ballot 返回一个 32-bit unsigned int，第 i 位对应 lane i 的条件结果
- popcount（population count）是硬件支持的操作，用于计数 bit 为 1 的个数；CUDA 提供 `__popc`
- stream compaction：先 ballot 得到掩码，再用 exclusive scan 计算每个 lane 的输出位置

### 复盘问题

- 如果同一 warp 的 16 个 lane 满足条件、16 个不满足，`__ballot_sync` 返回多少？
- stream compaction 中，popcount 的结果表示什么？
- 为什么说 `__activemask()` 在某些场景下不如显式 mask 准确？
- vote 和 ballot 相比 shared memory gather 的优势是什么？

### 对应官方参考

- CUDA C++ Programming Guide Section C.21: "Warp Vote Functions"
- CUDA C++ Programming Guide Section C.22: "Warp Shuffle Functions"（ballot 属于 shuffle 家族）
- CUDA C++ Best Practices: "Warp Vote Functions" section
- NVIDIA/cccl: `warp_scan` / `warp_ballot` examples

---

## 练习 D3：warp_reduce_scan

### 目标

综合 D1 和 D2，用 warp shuffle 和 ballot 高效实现 block-level reduce 和 scan，理解两层结构：warp-local 用 shuffle，block-level 用 shared memory。

### 前置理解

- 完成 D1（warp shuffle reduce/scan）和 D2（vote/ballot）
- 理解 block 由多个 warp 组成，需要两层操作才能完成 block-level 的 reduce/scan

### 必做任务

1. // TODO [必做] 实现一个两层 reduce：第一层用 warp shuffle 把每个 warp 的结果归到 lane 0，第二层用 shared memory + `__syncthreads` 汇合所有 warp 的结果。
2. // TODO [必做] 实现一个两层 scan：第一层在每个 warp 内用 Kogge-Stone 得到 inclusive scan，第二层在 warp 间用 smem 做"offset scan"，最后再加上 warp 间的 offset。
3. // TODO [必做] 对比三个版本的性能：(a) 全 shared memory，(b) 全 warp shuffle + atomic，(c) 两层混合。用 Nsight Compute 记录。
4. // TODO [必做] 验证 scan 结果（inclusive 和 exclusive）的正确性。
5. // TODO [必做] 测量不同 blockDim（128、256、512）下的吞吐差异。
6. // TODO [必做] 在 Nsight Compute 中查看瓶颈分类（是 memory-bound 还是 compute-bound）。

### 进阶任务

- 实现一个 block-level histogram（多个 reduce，每个对应一个 bin），用 ballot 快速判定 lane 的 bin 归属
- 尝试"split-K" reduce：多个 block 的结果用 global memory atomic 汇合

### 验收点

- 两层 reduce 和 scan 的结果与单层 smem 版本一致
- 性能对比显示两层混合方案在不同条件下的权衡
- Nsight Compute 瓶颈分类清晰可见
- 代码无寄存器溢出

### 观察点

- 两层结构的优势：warp-local shuffle 避免了 smem 访问的延迟与同步开销
- block-level reduce/scan 的最后汇合仍需 shared memory 和 `__syncthreads`
- 瓶颈往往从 smem 访问延迟（memory-bound）转向计算吞吐（compute-bound）

### 常见坑

- warp reduce 后没有 `__syncthreads` 就读 smem，导致 race
- 混淆 inclusive scan 和 exclusive scan 的含义（inclusive：包括自己；exclusive：不包括自己）
- block 内 warp 个数计算错误（blockDim.x / 32，向上取整）
- 忘记在最后一个 warp 的结果写入 smem 时做 sync

### 提示

- 两层 reduce 标准模式：`value = warp_reduce(value); if(laneId == 0) smem[warpId] = value; __syncthreads(); value = warp_reduce(smem[laneId] if laneId < warpCount else 0);`
- warp 内 scan 用 Kogge-Stone，warp 间 scan 需要额外的 offset（前面 warp 的 scan 输出值）
- 瓶颈判定：看 Nsight Compute 的 "SM Throughput" 是否接近 peak（接近 = compute-bound，否则 memory-bound）

### 复盘问题

- block-level reduce 为什么需要两层，而不是全用 smem？
- block-level scan 的"offset"是什么，为什么需要它？
- 如果 blockDim.x 不是 32 的倍数（例如 96），warp 间 reduce 如何处理最后一个不完整的 warp？
- 两层混合方案在什么情况下比全 smem 更优？

### 对应官方参考

- CUDA C++ Best Practices: "Reduction" and "Scan" sections
- NVIDIA/cccl: CUB library `BlockReduce` and `BlockScan`
- CUDA Samples: `reduction` sample (多个优化版本)

---

## 练习 D4：ldg_and_read_only_cache

### 目标

学会用 `__ldg` 指令和 read-only cache 优化只读数据的访问性能，理解什么时候 `__ldg` 有效、什么时候无效，以及如何用 Nsight Compute 验证。

### 前置理解

- 理解 GPU 内存层级（global → L2 → L1）
- 理解 `const __restrict__` 指针修饰的含义
- 理解 coalescing 和 cache 行为

### 必做任务

1. // TODO [必做] 写一个简单的 kernel：从 global memory 读只读数据（例如权重矩阵），进行计算。第一个版本用普通指针读，第二个版本用 `__ldg` 读。
2. // TODO [必做] 对两个版本进行性能对比测量（用 Nsight Compute 或计时）。
3. // TODO [必做] 在 PTX 代码中查看两个版本的指令差异：普通读是否是 `ld.global.ca`（with cache），`__ldg` 是否是 `ld.global.nc`（no cache）。
4. // TODO [必做] 尝试一个"memory-bound"场景（例如每个 thread 读 1 个 float，做最小计算）和"compute-bound"场景（例如每个 thread 读 1 个 float，做大量计算），观察 `__ldg` 效果的差异。
5. // TODO [必做] 用 `const __restrict__` 修饰指针，观察编译器是否能自动生成 `__ldg`（某些情况下可以）。
6. // TODO [必做] 在 Nsight Compute 中查看 L1 和 L2 cache 的命中率（hit ratio），对比优化前后。

### 进阶任务

- 实现一个 texture 读取版本（使用 texture cache），对比 `__ldg` 和 texture 的性能
- 尝试在 compute-bound kernel 中使用 `__ldg`，观察对性能的影响

### 验收点

- PTX 代码清晰显示 `__ldg` 生成了 `ld.global.nc` 指令
- 在 memory-bound 场景中，`__ldg` 相比普通读有明显加速
- Nsight Compute L1/L2 cache 指标显示差异
- compute-bound 场景中 `__ldg` 效果不明显（符合预期）

### 观察点

- `__ldg` 绕过 L1 cache，直接访问 L2 和 global memory，在某些模式下更优
- `__ldg` 最优场景是"稀疏、非临时"的只读访问（例如权重矩阵的不规则访问）
- `const __restrict__` 给编译器优化的机会，但不保证生成 `__ldg`
- read-only cache 是一种特殊的片上缓存，行为与通用 L1 不同

### 常见坑

- 在 compute-bound kernel 中期望 `__ldg` 有巨大性能提升（实际效果有限）
- 混淆 `__ldg` 和 texture：`__ldg` 用硬件的 read-only cache，texture 有专门的 texture cache（二者独立）
- 忘记 `#include <cuda_runtime_helpers.h>` 或类似头文件，导致 `__ldg` 不可用
- `__ldg` 仅对指针参数有效，如果数据已在寄存器中则无效
- 没有实际测量性能，只是猜测 `__ldg` 是否有效

### 提示

- `__ldg` 原型：`template<class T> T __ldg(const T* ptr);` 任何标量类型都可
- 查看 PTX：`nvcc -arch sm_90a -ptx kernel.cu` 生成 .ptx 文件，搜索 `ld.global`
- memory-bound 验证：Nsight Compute 中看 "Memory Throughput / Peak Throughput" 的比值
- `const __restrict__` 写法：`const float * __restrict__ weights` 表示只读且无别名

### 复盘问题

- `__ldg` 为什么在 memory-bound 场景下有效，在 compute-bound 场景下无效？
- read-only cache 和 L1 cache 的主要区别是什么？
- 如果一个指针既读又写，`__ldg` 还能用吗？
- `const __restrict__` 修饰的指针一定会生成 `__ldg` 吗？

### 对应官方参考

- CUDA C++ Programming Guide Section 3.2.2: "Device Memory"
- CUDA Runtime API: `__ldg` function
- CUDA C++ Best Practices: "Read-Only Data Cache" section
- PTX ISA: `ld.global.*` instruction variants

---

## 练习 D5：occupancy_and_launch_bounds

### 目标

理解 occupancy 的定义、计算、与性能的关系，以及如何用 `__launch_bounds__` 属性和 `cudaOccupancyMaxPotentialBlockSize` API 调控 blockDim、寄存器压力和 occupancy 的权衡。

### 前置理解

- 理解 block/warp/thread 映射和 SM 资源（寄存器、smem、并发 warp 数）
- 理解寄存器溅出（spill）的概念

### 必做任务

1. // TODO [必做] 写一个 kernel 不添加任何 attribute，用 `cudaOccupancyMaxPotentialBlockSize` 查询推荐的 blockDim。输出查询结果。
2. // TODO [必做] 在 kernel 前添加 `__launch_bounds__(maxThreadsPerBlock, minBlocksPerMultiprocessor)` attribute，尝试三组值：(256, 0)、(256, 2)、(256, 4)。
3. // TODO [必做] 对于每一组，运行 kernel 并在 Nsight Compute 中记录 occupancy 和寄存器使用量。
4. // TODO [必做] 对比三个版本的性能（吞吐或延迟），找出最优的 attribute 设置。
5. // TODO [必做] 实现一个寄存器压力较大的 kernel（例如每个 thread 用 80+ 寄存器），观察 spill 如何发生、如何影响性能。
6. // TODO [必做] 用 `nvcc -Xptxas -v` 编译，查看寄存器使用统计。

### 进阶任务

- 实现一个递归或循环的 kernel，逐步增加寄存器用量，绘制"occupancy vs 寄存器数"的曲线
- 尝试用 `#pragma unroll` 控制循环展开，观察对寄存器和 occupancy 的影响

### 验收点

- `cudaOccupancyMaxPotentialBlockSize` 的推荐值合理（根据寄存器和 smem 用量）
- `__launch_bounds__` 的不同参数组合显示 occupancy 的变化趋势
- Nsight Compute 数据清晰显示 occupancy 与性能的关系
- 寄存器 spill 时能在 PTX 中看到 `st.local` / `ld.local` 指令

### 观察点

- occupancy 高不一定性能好（取决于瓶颈）
- `__launch_bounds__(maxThreads, minBlocks)` 中 minBlocks 提示编译器至少要保证多少 block 同时运行，以此优化寄存器分配
- 寄存器 spill 到 local memory（实际上是 global memory）会导致巨大延迟
- 某些情况下降低 occupancy 反而能减少寄存器压力，提升性能

### 常见坑

- 盲目追求 occupancy = 100%，导致寄存器 spill
- `__launch_bounds__` 的 minBlocks 设置过高，导致编译器无法满足而忽略 attribute
- 混淆"理论 occupancy"（资源限制）和"实际 occupancy"（运行时实测）
- 在 blockDim 远小于 SM 资源能承载的情况下追求 occupancy（此时 occupancy 自动很高）
- 没有用 `__restrict__` 等指令帮助编译器优化，导致寄存器用量不必要的高

### 提示

- occupancy 定义：`(活跃 warp 数) / (每 SM 最大 warp 数)`，sm_90a 时每 SM 最多 128 warp
- `cudaOccupancyMaxPotentialBlockSize` 会考虑动态 smem，如果用了动态 smem 需在调用时指定大小
- 寄存器使用统计：`nvcc -Xptxas -v kernel.cu 2>&1 | grep registers` 
- local memory 用量查看：`nvcc -Xptxas -v kernel.cu 2>&1 | grep "local memory"`

### 复盘问题

- sm_90a 上，如果 occupancy 100% 但性能反而下降，可能的原因是什么？
- `__launch_bounds__(256, 2)` 意味着什么？编译器会如何调整寄存器分配？
- occupancy 从 50% 提升到 100% 一定能使吞吐翻倍吗？
- 如何判断当前 kernel 是被 occupancy 限制还是被其他因素（memory/compute）限制？

### 对应官方参考

- CUDA C++ Programming Guide Section 4.2: "Hardware Multithreading"
- CUDA Runtime API: `cudaOccupancyMaxPotentialBlockSize`
- CUDA C++ Best Practices: "Execution Configuration Optimizations" section
- Nsight Compute: "Occupancy Analysis" section

---

## 练习 D6：bottleneck_classification

### 目标

学会用 Nsight Compute 的指标和 roofline 模型，对给定的 kernel 进行瓶颈分类（compute-bound / memory-bound / latency-bound），并提出有针对性的优化方向。

### 前置理解

- 完成前五个练习，理解 warp 原语、occupancy 等概念
- 理解 roofline 模型的基础：peak throughput（计算或内存）、arithmetic intensity（计算量/内存访问量）
- 理解"Speed of Light"分析（能达到的最大吞吐）

### 必做任务

1. // TODO [必做] 给定三个典型 kernel：(a) reduction（memory-bound），(b) matrix tile multiply（compute-bound），(c) indirect memory access（latency-bound），分别用 Nsight Compute `--set full` 运行。
2. // TODO [必做] 对每个 kernel 记录：achieved throughput、peak throughput、L2 bandwidth、achieved occupancy。
3. // TODO [必做] 用 Nsight Compute 的"Speed of Light"部分判定每个 kernel 的瓶颈。记录 SM Throughput、L1 Throughput、L2 Throughput、DRAM Throughput 四个指标中哪个最接近 peak。
4. // TODO [必做] 根据瓶颈分类，为每个 kernel 提出一个可行的优化方向（例如 memory-bound 可增加 arithmetic intensity，compute-bound 可增加 occupancy）。
5. // TODO [必做] 绘制或记录三个 kernel 在 roofline 图上的位置（横轴：arithmetic intensity，纵轴：throughput）。观察它们分别落在"计算墙"还是"内存墙"一侧。
6. // TODO [必做] 对其中一个 kernel 实施一个优化，再次运行 Nsight Compute，对比优化前后的指标。

### 进阶任务

- 实现一个混合 kernel（既有 compute 也有 memory），分析其复杂的瓶颈特性
- 对比不同硬件（sm_80 vs sm_90a）上同一 kernel 的瓶颈分类差异

### 验收点

- 三个 kernel 的 roofline 分析结果清晰，分别落在不同象限
- 瓶颈分类与 Nsight Compute "Speed of Light"指标一致
- 优化建议具体可行（例如"减少 L2 访问通过 shared memory tiling"）
- 优化后的数据显示相应指标的改善

### 观察点

- memory-bound kernel 的吞吐被 L2/DRAM 带宽限制，occupancy 高也帮不了
- compute-bound kernel 的吞吐被 SM 计算能力限制，需要增加 occupancy 或减少 warp divergence
- latency-bound kernel（如不规则内存访问）的瓶颈难以直观判断，需要关注 L1/L2 miss 率和 pipeline stall
- roofline 图能快速定位优化方向

### 常见坑

- 混淆"achieved throughput"和"peak throughput"（achieved 是实际，peak 是理论最大）
- 在 memory-bound kernel 上追求 occupancy 优化（没有帮助）
- 没有考虑到"实际可达的吞吐"受多个指标共同限制（例如既受 L2 带宽也受 DRAM 延迟）
- 在 roofline 图上读错坐标，误判瓶颈
- 对非 compute-heavy 的 kernel 盲目应用 CUTLASS 等高度优化库（可能没有对应的专化实现）

### 提示

- arithmetic intensity = (float ops) / (bytes accessed)，越高越偏向 compute-bound
- 常见阈值（sm_90a）：intensity > ~10 时 compute-bound，< 1 时 memory-bound，1–10 之间看其他因素
- Nsight Compute 的"Memory Throughput / Peak Memory Throughput"接近 100% = memory-bound
- Nsight Compute 的"SM Throughput / Peak SM Throughput"接近 100% = compute-bound

### 复盘问题

- 如果一个 kernel 的 roofline 点正好在计算墙和内存墙的交界处，说明什么？
- latency-bound kernel 应该如何优化？
- 为什么同一个 kernel 在 sm_80 和 sm_90a 上的瓶颈可能不同？
- 能否构造一个"完全 latency-bound"的 kernel？

### 对应官方参考

- Nsight Compute Documentation: "Kernel Profiling Guide"
- "Roofline Model" paper (Williams et al.)
- CUDA C++ Best Practices: "Performance Metrics" section
- NVIDIA blog posts on roofline analysis

---

## 做完本模块后应达到的水平

**知识检查清单**

- 能用五大 warp shuffle 函数实现 warp-level reduce 和 scan
- 理解 vote 和 ballot 的语义，能用 ballot 实现 stream compaction
- 能设计两层结构的 block-level reduce/scan，理解 warp-local 和 block-level 的权衡
- 知道何时使用 `__ldg` 以及它的局限性
- 能用 `cudaOccupancyMaxPotentialBlockSize` 和 `__launch_bounds__` 调节 occupancy
- 理解 occupancy 与性能的关系（高 occupancy 不等于高性能）
- 能用 Nsight Compute 进行瓶颈分类（compute-bound / memory-bound / latency-bound）
- 理解 roofline 模型并能在图上定位一个 kernel

**能做到**

- 手写高效的 warp-local 算子（reduce、scan、compaction）
- 结合 warp shuffle 和 shared memory 优化 block-level 操作
- 用 Nsight Compute 的"Speed of Light"指标快速判定瓶颈
- 根据瓶颈提出有针对性的优化方向

**见模块 E** 关于流、事件和 CUDA Graph 的并发执行；**见模块 C** 关于同步原语的深度；**见模块 F** 关于 Nsight 工具的全面讲解。
