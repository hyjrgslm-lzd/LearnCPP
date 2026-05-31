# 练习 C4：hopper_cluster_launch

## 目标

`[C4-T01]` (main.cu:1) 文件标识：`C4_hopper_cluster_launch/main.cu`。
`[C4-T02]` (main.cu:2) 练习 C4：Hopper Thread Block Cluster — 编译期声明 + 运行期启动 + 分布式 smem。
`[C4-T03]` (main.cu:4) 硬件要求：sm_90a（Hopper），target 已在 `CMakeLists.txt` 中显式设置 `CUDA_ARCHITECTURES "90a"`。
`[C4-T04]` (main.cu:7) 学习目标：`__cluster_dims__(2,1,1)` 编译期声明；`cudaLaunchKernelEx` + `cudaLaunchConfig_t` 运行期启动；`cluster.map_shared_rank` 访问相邻 block 的 smem；`cluster.sync()` 跨 block 屏障。
`[C4-T05]` (main.cu:13) 编译、运行与 Nsight Compute 命令。

理解 Hopper 新增的 thread block cluster 机制：编译期用 `__cluster_dims__` 声明、运行期用 `cudaLaunchKernelEx` 启动、多个 block 通过分布式 smem 协作。这是为后续 TMA + warp specialization pipeline（模块 G/H）打基础。

## 前置理解

- 完成练习 C3，掌握基础同步
- 理解 Hopper (sm_90+) 的硬件特性
- 理解普通 block 只能用 `__syncthreads` 同步，不能跨 block 访问彼此的 smem

## 必做任务

`[C4-T06]` (main.cu:21) `cooperative_groups` 提供 cluster group 抽象。
`[C4-T07]` (main.cu:30) 常量定义。
`[C4-T08]` (main.cu:33) 每 block 使用的 smem int 个数。
`[C4-T09]` (main.cu:34) cluster 由 2 个 block 组成（x 方向）。
`[C4-T10]` (main.cu:37) Kernel：cluster launch 演示，编译期用 `__cluster_dims__(2,1,1)`，运行期通过 `cudaLaunchKernelEx` 指定（两者必须一致）。
`[C4-T11]` (main.cu:42) TODO [必做-1] `__cluster_dims__` 声明。
`[C4-T12]` (main.cu:43) TODO [必做-3] 读取 `clusterIdx` / `clusterSize`。
`[C4-T13]` (main.cu:44) TODO [必做-4] 分布式 smem 访问。
`[C4-T14]` (main.cu:47) TODO [必做-1] 在下一行取消注释，加上编译期 cluster 声明。
`[C4-T15]` (main.cu:51) TODO [必做-3] 获取 cluster group。
`[C4-T16]` (main.cu:58) `block_id`：block 在 grid 中的全局索引。
`[C4-T17]` (main.cu:61) 初始化 smem：每个 block 写入自己的 `block_id * 100 + tid`。
`[C4-T18]` (main.cu:67) TODO [必做-4] `cluster.sync()`：等待 cluster 内所有 block 完成 smem 初始化。
`[C4-T19]` (main.cu:72) TODO [必做-4] 访问相邻 block（XOR 1）的 smem。
`[C4-T20]` (main.cu:78) TODO [必做-5] 验证分布式 smem 地址计算。
`[C4-T21]` (main.cu:83) TODO 修复后写 `peer_val`；现在写 stub=0 使验收点可见。
`[C4-T22]` (main.cu:87) TODO [必做-3] 打印 cluster / block 信息（每个 block 的 lane 0）。
`[C4-T28]` (main.cu:114) TODO [必做-2] 用 `cudaLaunchKernelEx` 启动（取代 `<<<...>>>`）。
`[C4-T29]` (main.cu:134) 临时用传统启动（TODO 完成后替换为上面的 `cudaLaunchKernelEx`）。
`[C4-T31]` (main.cu:141) 验证输出（TODO 完成后检查分布式 smem 数据）。
`[C4-T33]` (main.cu:149) TODO [必做-6] Nsight Compute 验证 cluster 启动。

1. 在 kernel 定义前加 `__cluster_dims__(2,1,1)` 声明，标记该 kernel 希望以 2×1×1 的 cluster 运行。编译并验证不报错。
2. 用 `cudaLaunchKernelEx` 而非 `<<<...>>>` 启动，并在 launch 时指定 cluster size（需用 `cudaLaunchConfig_t` 和 `cudaLaunchAttribute_t`）。
3. 在 kernel 内部用 `__cluster_dims__` 读取编译期声明，用 `__clusterIdx` 和 `__clusterSize` 获取运行期信息。打印每个 cluster 中每个 block 的信息。
4. 实现分布式 smem 访问：block 0 往自己的 smem 写数据，block 1 用 `cluster_shared_memory_ptr` 读 block 0 的 smem。在 `cluster.sync()` 处做同步。
5. 验证分布式 smem 的地址计算。给定另一 block 的偏移，能否准确访问到它的数据？
6. 在 Nsight Compute 中验证 cluster 确实被启动了（看 block 分布或相关指标）。

## 进阶任务

`[C4-T34]` (main.cu:157) 进阶 Kernel 2：cluster 内 all-reduce stub。
`[C4-T35]` (main.cu:158) TODO [进阶] `clusterDim=(2,2,1)`，4 个 block 共享数据。
`[C4-T37]` (main.cu:162) all-reduce 步骤说明：每个 block 贡献一个值，cluster 内汇合到 block 0 的 `smem[0]`。
`[C4-T38]` (main.cu:170) TODO [进阶] 实现上述逻辑，期望结果 = 1 + 2 = 3（2-block cluster）。
`[C4-T40]` (main.cu:176) 进阶 Kernel 3：`clusterDim=(2,2,1)` — 4 block 共享。
`[C4-T41]` (main.cu:177) TODO [进阶]。
`[C4-T43]` (main.cu:181) 启动配置：`gridDim=(4,1,1)`，`clusterDim=(2,2,1)`。
`[C4-T44]` (main.cu:185) TODO [进阶] 验证 4-block cluster 中分布式 smem 地址（block (0,0) 读 block (1,1) 的 smem，block (0,1) 读 block (1,0) 的 smem）。
`[C4-T46]` (main.cu:192) 手动验证分布式 smem 地址计算（无 GPU，仅打印公式）。

- 尝试 blockDim=(256,1,1), clusterDim=(2,2,1)，观察 4 个 block 共享数据的场景
- 实现一个简单的 all-reduce across cluster：每个 block 贡献一个值，cluster 内汇合

## 验收点

`[C4-T24]` (main.cu:93) 主程序入口。
`[C4-T25]` (main.cu:100) Hopper 特性检测。
`[C4-T27]` (main.cu:108) `N = BLOCK_SIZE * CLUSTER_DIM_X`，即 2 个 block，共 N 个线程。

- kernel 编译通过，运行时正确启动了 cluster
- 分布式 smem 访问读出正确的数据
- `cluster.sync()` 能正确同步 cluster 内所有 block
- Nsight Compute 在"cluster mode"下能显示 block 归属关系

## 观察点

- cluster 是 Hopper 新增的"超 block"组织，允许多个 block 共享地址空间
- 分布式 smem 地址计算需要知道另一 block 的偏移；CUDA 提供了 helper 函数
- cluster 对于 producer-consumer warp specialization 至关重要（生产者 block 用 TMA 搬数据，消费者 block 做计算）
- `cluster.sync()` 等价于跨多个 block 的屏障

## 常见坑

- 忘记在 kernel 前加 `__cluster_dims__` 编译期声明，只在运行期指定，可能无法获得编译器优化
- cluster 大小超过 8 block（硬件限制），导致 launch 失败
- 混淆 `clusterIdx` 和 `blockIdx`：`clusterIdx` 是 cluster 在 grid 中的位置，`blockIdx` 是 block 在 cluster 中的位置
- 在分布式 smem 访问前没有 `cluster.sync()`，导致读到初始化前的垃圾数据
- `cluster_shared_memory_ptr` 计算错误，导致访问越界或看不到数据
- 假设 cluster 内的 block 会并行执行，实际上不保证（hardware 可能序列化）
- 在非 sm_90 硬件上尝试 cluster，导致编译或链接失败

## 提示

- cluster 声明语法：`__cluster_dims__(x, y, z)` 必须是常量，且 x*y*z <= 8
- 运行期配置：`cudaLaunchConfig_t launchConfig = {}; launchConfig.gridDim = gridDim; launchConfig.blockDim = blockDim; launchConfig.clusterDim = {clusterDimX, clusterDimY, clusterDimZ};`
- 访问另一 block 的 smem：`char* other_smem = (char*)__to_shared(blockIdx.x ^ 1, smem_ptr);`（XOR 是为了切换到相邻 block，实际需根据逻辑调整）
- cluster.sync() 会发出 `barrier.cluster.arrive / wait` 指令

## 复盘问题

- 编译期 `__cluster_dims__` 和运行期 `cudaLaunchKernelEx` 指定的 cluster 大小不一致时会怎样？
- 分布式 smem 访问相比普通 global memory 访问有什么优势？
- cluster 内两个 block 最多相隔多少个字节的 smem？
- 为什么 cluster 对后续 TMA + warp specialization 有重要意义？

## 对应官方参考

- CUDA C++ Programming Guide Section 2.7: "Thread Block Clusters (Hopper+)"
- CUDA C++ Programming Guide Section 3.2.7: "Cluster Group"
- Hopper Tuning Guide: "Thread Block Clusters" section
- CUDA Runtime API: `cudaLaunchKernelEx`, `cudaLaunchConfig_t`

## 输出对照（printf / std::puts 原文）

- `[C4-T23]` (main.cu:89) 原文：`block_id=%d  (cluster info: TODO [必做-3])` → 现：`block_id=%d  (cluster info: TODO [REQUIRED-3])`
- `[C4-T26]` (main.cu:102) 原文：`[SKIP] sm_90a not available on this device. Hopper cluster skipped.` → 现：保持英文不变
- `[C4-T30]` (main.cu:135) 原文：`启动配置: grid=%d block=%d (cluster=%d×1×1)` → 现：`Launch config: grid=%d block=%d (cluster=%dx1x1)`
- `[C4-T32]` (main.cu:146) 原文：`out[0]=%d out[1]=%d ... (stub=0，完成 TODO 后应为 peer smem 值)` → 现：`out[0]=%d out[1]=%d ... (stub=0; after the TODO, should be peer smem values)`
- `[C4-T36]` (main.cu:160) 原文：`--- 进阶：cluster all-reduce stub ---` → 现：`--- Advanced: cluster all-reduce stub ---`
- `[C4-T39]` (main.cu:172) 原文：`  [进阶 stub] cluster all-reduce 未实现，期望 out[0]=3` → 现：`  [advanced stub] cluster all-reduce not implemented; expected out[0]=3`
- `[C4-T42]` (main.cu:179) 原文：`--- 进阶：2×2 cluster（4 blocks）---` → 现：`--- Advanced: 2x2 cluster (4 blocks) ---`
- `[C4-T45]` (main.cu:188) 原文：`  [进阶 stub] 2×2 cluster 未实现` → 现：`  [advanced stub] 2x2 cluster not implemented`
- `[C4-T47]` (main.cu:194) 原文：`--- 分布式 smem 地址计算说明 ---` → 现：`--- Distributed smem address calculation ---`
- `[C4-T48]` (main.cu:195) 原文：`  cluster 内 block rank 0 的 smem 基地址 = smem_ptr（本地）` → 现：`  base address of smem in cluster block rank 0 = smem_ptr (local)`
- `[C4-T49]` (main.cu:196) 原文：`  cluster 内 block rank 1 的 smem 基地址 = cluster.map_shared_rank(smem_ptr, 1)` → 现：`  base address of smem in cluster block rank 1 = cluster.map_shared_rank(smem_ptr, 1)`
- `[C4-T50]` (main.cu:197) 原文：`  访问 peer block smem 的第 k 个元素：peer_smem[k]` → 现：`  access element k of peer block smem: peer_smem[k]`
- `[C4-T51]` (main.cu:198) 原文：`  注意：peer_smem 必须在 cluster.sync() 之后才能安全读取` → 现：`  Note: peer_smem may only be read safely after cluster.sync()`
- `[C4-T52]` (main.cu:201) 原文：`[C4] 完成。用 Nsight Compute 验证 cluster 启动与分布式 smem。` → 现：`[C4] done. Use Nsight Compute to verify cluster launch and distributed smem.`
