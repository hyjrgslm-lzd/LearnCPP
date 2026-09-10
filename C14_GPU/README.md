# C14：CUDA 与 GPU 编程

## 这套文档要解决什么问题

这不是一套"背 CUDA API"的笔记，而是一套"通过亲手编码理解 GPU 硬件模型与高性能算子设计"的练习包。GPU 编程有两个容易割裂的维度：一是 CUDA 语言扩展（kernel 语法、内存模型、同步原语），二是硬件执行模型（SM 调度、warp 流水、内存层级带宽、Tensor Core 流水线）。只有同时建立这两个维度的直觉，你才能写出不只是"能跑"、而是"真正高效"的 kernel。本套文档从零 CUDA 起点出发，逐步覆盖 warp 原语、shared memory、TMA、Tensor Core、CUTLASS/cuTe、AI Infra 算子，直至 OptiX 光追管线，以 Hopper (sm_90a) 为主线、Blackwell (sm_100a/120a) 为进阶、Ampere/Ada (sm_80/86/89) 为回退对比。

## 你会得到什么

这套练习包分为两个阶段：

### 第一阶段：使用层——从零 CUDA 到独立写中等规模 kernel

- 1 份心智模型总说明（模块 01）。
- 6 个模块（A/B/C/D/E/F），合计约 31 道练习题。
- 1 个结课项目（数据并行管道 + Nsight 剖析报告）。
- 每题统一的 timeline 草图 + Nsight Compute 指标 + 复盘框架。

### 第二阶段：实现层——写接近 SOTA 的算子，读 CUTLASS/FA 源码

- 5 个模块（G/H/I/J/K），合计约 27 道练习题。
- 1 个三分支结课项目：CUTLASS 精读 + FlashAttention 复现 + OptiX mini 渲染器。
- 覆盖 wgmma/TMA、CUTLASS 3.x 五层 hierarchy、FP8 量化、cuDNN v9 graph、NCCL、OptiX 8 管线。

### 总计

**约 58 道练习题 + 2 个结课项目 + 14 份模块文档**

## 阅读顺序

### 第一阶段

1. `01-心智模型.md`
2. `02-模块A-环境与首个kernel.md`
3. `03-模块B-内存层级与数据移动.md`
4. `04-模块C-线程层级与同步原语.md`
5. `05-模块D-Warp原语与占用率.md`
6. `06-模块E-流事件与CUDA-Graph.md`
7. `07-模块F-Nsight与性能工程.md`
8. `08-结课项目1-数据并行管道与性能基线.md`

### 第二阶段

9. `09-模块G-TensorCore-wgmma-TMA.md`
10. `10-模块H-GEMM优化阶梯与CUTLASS.md`
11. `11-模块I-AI算子手写实战.md`
12. `12-模块J-生态库与AI-Infra.md`
13. `13-模块K-OptiX光线追踪管线.md`
14. `14-结课项目2-源码精读与mini实现.md`

## 统一技术基线

本练习包绑定如下工具链，所有题目在此边界内保证可编译运行：

- **宿主语言基线**：C++26，MSVC `/std:c++latest /Zc:__cplusplus /utf-8 /permissive-`
- **设备语言基线**：CUDA C++20（nvcc 13.x 稳定支持上限）
- **编译器**：Visual Studio 2026，MSVC 19.5x
- **CUDA Toolkit**：13.x
- **CMake**：3.28+，`LANGUAGES CXX CUDA`
- **硬件基线**：
  - 主线：Hopper sm_90a（wgmma / TMA / cluster / mbarrier 全功能）
  - 进阶：Blackwell sm_100a / sm_120a（MXFP8/FP4、新 MMA 指令）
  - 回退对比：Ampere sm_80 / Ada sm_86 sm_89（FP8 E4M3/E5M2 需 sm_89+）
  - CMake 目标：`CMAKE_CUDA_ARCHITECTURES "80;86;89;90a;100a;120a"`
- **外部依赖**：
  - CUTLASS 3.x：FetchContent（header-only，镜像 stdexec 做法）
  - cuBLAS / cuDNN / NCCL / NVTX：CUDA Toolkit 随附
  - OptiX 8 SDK：手动安装（NV 许可协议禁止 FetchContent）

## 两个阶段的定位差异

### 第一阶段：你学的是"怎么用 GPU"

- 理解 Host/Device 执行模型与 SIMT 并发
- 掌握 global / shared / constant / register 内存各层读写语义
- 理解 grid / block / warp / lane 四级线程层级及其映射关系
- 掌握 `__syncthreads`、cooperative groups、Hopper cluster 同步
- 能用 Nsight Compute 定位 memory-bound / compute-bound 瓶颈
- 能手写 reduce、scan、histogram 并达到可测量的带宽利用率

### 第二阶段：你学的是"怎么写接近 SOTA 的算子"

- 理解 Tensor Core 指令层级：wmma → mma.sync → wgmma（Hopper）
- 掌握 TMA（Tensor Memory Accelerator）异步数据移动与 producer-consumer warp specialization
- 能用 CUTLASS 3.x 的 CollectiveBuilder + GemmUniversalAdapter 搭建高性能 GEMM
- 理解 cuTe 的 Layout / Tensor / Copy_Atom / MMA_Atom 四大概念
- 能手写 online softmax、layernorm、fused attention，理解 FlashAttention-2/3 思想
- 能调用 cuDNN v9 graph API、cuBLASLt epilogue fusion、NCCL allreduce
- 能搭建 OptiX 8 raygen/hit/miss/SBT 管线并输出可渲染图像

## 你要始终记住的定位

### 1. CUDA 有两个不能混淆的维度

- 语言扩展维度：`__global__`、`__device__`、`<<<grid,block>>>`、`__shared__`、PTX 内联——这是编译器的工作，属于语法层。
- 硬件执行维度：SM 数量、warp 调度器数量、L1/L2 容量、内存带宽、Tensor Core 吞吐——这是芯片的工作，属于性能层。
写出高效 kernel 要求你同时理解两层，并能把语言特性映射到硬件资源上。

### 2. 本地练习时，代码命名会分成两层

- 标准概念层：写作 CUDA 编程模型（grid/block/warp/lane，全局内存/共享内存，流/事件）
- 架构特性层：代码使用 `sm_90a` / `sm_100a` 特性标注，文档中明确标出 compute capability 门槛

### 3. 这套文档不追求"最短可运行代码"

它追求的是：
- 你能把一个算子的数据流分解为：全局读 → 共享内存暂存 → 寄存器计算 → 全局写
- 你能说清每一步的内存层级、带宽瓶颈、同步点
- 你能说清线程层级的每一维为什么这样选（block size、grid size、warp tile）
- 你能从 Nsight Compute 的 roofline 图上读出"这个 kernel 离峰值还差多少、差在哪里"
- （第二阶段）你能解释 CUTLASS collective 的设计选择，并能实现其中的关键子集

### 4. 这套文档不为以下目标优化

- 跨平台兼容（Linux / macOS CUDA 路径有差异，不在覆盖范围）
- 最低配置运行（某些 Stage 2 题目需要 Hopper 或 Blackwell 硬件）
- 生产级错误恢复（练习代码用 `CUDA_CHECK` abort，不写重试逻辑）

## 每题统一交付物

每完成一道题，至少留下五样东西：

1. 一份可运行的 `main.cu`（TODO 骨架填写完整）。
2. 一张 timeline 草图（kernel launch 时序、stream 并发关系、或 warp 流水图）。
3. Nsight Compute 关键指标截图或数值记录（内存带宽利用率、Tensor Core 利用率、achieved occupancy）。
4. 一段 5 到 10 行的观察记录（亲眼测量到的现象，不是复述文档）。
5. 一段复盘结论：这一题到底让你理解了什么硬件行为或设计取舍。

## 每题统一模板

所有练习题都按同一模板组织：

- 目标
- 前置理解
- 必做任务（含 `// TODO [必做]` 标记）
- 进阶任务（含 `// TODO [进阶]` 标记）
- 验收点
- 观察点
- 常见坑
- 提示
- 复盘问题
- 对应官方参考

你做题时也尽量按这个模板留笔记。这样你回看时，会非常容易发现自己到底卡在"硬件没理解"、"API 没用熟"，还是"性能模型没建立"。

## 统一判定标准

如果你做完一道题，只是"代码跑了"，那还不够。每道题必须能回答下面六个问题：

1. **线程层级**：本题 grid × block × warp × lane 各维度分别是多少？为什么这样选？
2. **内存路径**：数据经过了 global → L2 → L1/shared → register 哪些层级？有没有绕过某层？
3. **瓶颈分类**：这个 kernel 是 compute-bound、memory-bound 还是 latency-bound？Nsight Compute 里哪个指标是证据？
4. **同步原语**：`__syncthreads`、warp-sync（`__syncwarp`）、`mbarrier`、`cluster.sync()` 分别在哪里出现，为什么必须在那里？
5. **占用率**：`achieved occupancy` 是多少？每线程寄存器数量、smem 用量分别是多少？限制因素是什么？
6. **数值精度**：本题用了哪种 Tensor Core 指令（wmma/mma.sync/wgmma）？输入精度、累加精度、输出精度各是什么？

第二阶段额外检查：
- 你能说清 CUTLASS Collective 里 TiledMMA 和 TiledCopy 分别负责什么、怎么组合的。
- 你能说清 cuTe Layout 是如何把逻辑坐标映射到物理地址的。
- 你能说清本题算子与 cuBLAS / cuDNN 参考实现的性能差距以及原因。

## 建议节奏

### 方案 A：第一阶段 12-16 天

- 第 1 天：`01-心智模型.md`
- 第 2-3 天：模块 A（环境、首个 kernel、PTX/SASS 查看）
- 第 4-5 天：模块 B（内存层级、合并访问、bank conflict）
- 第 6-7 天：模块 C（线程同步、cooperative groups、Hopper cluster）
- 第 8-9 天：模块 D（warp 原语、占用率、`__launch_bounds__`）
- 第 10-11 天：模块 E（stream、event、CUDA Graph）
- 第 12-13 天：模块 F（Nsight Systems、Nsight Compute、roofline）
- 第 14-16 天：结课项目 1

### 方案 B：第二阶段 14-20 天

- 第 1-2 天：模块 G（Tensor Core、wgmma、TMA 基础）
- 第 3-5 天：模块 H（GEMM 优化阶梯、CUTLASS 3.x、cuTe）
- 第 6-8 天：模块 I（AI 算子手写：softmax/layernorm/attention）
- 第 9-11 天：模块 J（cuBLAS/cuDNN v9/NCCL/TensorRT/TE）
- 第 12-13 天：模块 K（OptiX 8 管线）
- 第 14-20 天：结课项目 2（三分支选一或全做）

### 方案 C：慢练，全程约 6 周

- 每天只做 1 道练习题
- 每做完一个模块，安排一天专门跑 Nsight Compute 回看
- 结课项目各预留 3 天

## 术语速查

| 术语 | 含义 | 在练习里会看到什么 |
|---|---|---|
| grid | kernel 启动的最高级线程组织，包含若干 block | `<<<gridDim, blockDim>>>` |
| block | 一组共享 shared memory 和 `__syncthreads` 的线程 | `blockIdx.x * blockDim.x + threadIdx.x` |
| warp | SM 调度的最小单位，32 个线程，同步 SIMT | `threadIdx.x / 32` 为 warp ID |
| lane | warp 内编号 0–31 的单个线程 | `threadIdx.x % 32` 为 lane ID |
| SM | 流式多处理器（Streaming Multiprocessor），GPU 的执行核心 | Nsight Compute 里的 SM utilization |
| coalescing | 同一 warp 的全局内存访问合并为单次事务 | 连续地址 128-byte 对齐可合并 |
| bank conflict | shared memory 中同一 warp 内多个线程访问同一 bank | 导致串行化，Nsight 报 shared_ld_bank_conflict |
| occupancy | SM 上活跃 warp 数 / 最大 warp 数 | Nsight Compute 里的 achieved_occupancy |
| `__syncthreads` | block 内全体线程的屏障，等待所有线程到达 | shared memory 写后必须调用再读 |
| cooperative groups | 灵活的线程组同步 API，支持 block/tile/grid 多级 | `this_thread_block()`, `tiled_partition<32>()` |
| cluster (sm_90+) | Hopper 新增的 block 上一级组织，同一 cluster 的 block 可访问彼此 shared memory（分布式 smem） | `__cluster_dims__(2,1,1)`, `cluster.sync()` |
| mbarrier | Hopper 引入的异步 barrier，支持 transaction count 追踪 | `cuda::barrier<cuda::thread_scope_block>` |
| TMA / `cp.async.bulk.tensor` | Tensor Memory Accelerator，硬件单元负责异步批量搬运张量，需 CC ≥ 9.0 | `cuTensorMapEncodeTiled` + `ptx::cp_async_bulk_tensor` |
| WMMA / MMA / WGMMA | Tensor Core 三代 API：wmma（sm_70+）→ mma.sync PTX（sm_75+）→ wgmma（sm_90+） | 精度从 FP16 扩展到 BF16/TF32/FP8 |
| FP8 E4M3 / E5M2 | 8-bit 浮点格式，E4M3 范围小精度高，E5M2 范围大；需 sm_89+（Ada/Hopper） | `__nv_fp8_e4m3`, per-tensor scaling |
| CUTLASS Collective | CUTLASS 3.x 五层 hierarchy 中第三层，封装 TiledMMA + TiledCopy + epilogue | `CollectiveBuilder`, `GemmUniversalAdapter` |
| cuTe Layout | cuTe 的核心数据结构，把逻辑坐标映射到物理内存地址，支持任意 stride | `make_layout(Shape, Stride)` |
| CUDA Graph | 预先捕获 kernel 序列为图，replay 时消除 launch overhead | `cudaStreamBeginCapture` + `cudaGraphInstantiate` |
| Nsight Compute | kernel 级性能分析工具，提供 roofline、source/SASS 关联、内存层级分析 | `ncu --set full ./my_kernel` |
| Nsight Systems | 系统级时间线工具，可视化 stream 并发、CPU/GPU 交互、NVTX 标注 | `nsys profile --trace cuda,nvtx ./app` |
| NVTX range | NVIDIA Tools Extension 标注 API，在 Nsight Systems 时间线上插入命名区间 | `nvtxRangePushA("gemm")` / `nvtxRangePop()` |

## 统一编码约束

- 每道题先写 naive 版本，验证正确性后再做性能优化；不要把 naive 和 optimized 写在同一个分支。
- 每次 kernel launch 前打印 grid / block / smem 配置，方便 Nsight Compute 关联核验。
- 所有 CUDA runtime / driver API 调用必须用 `CUDA_CHECK` 宏包裹，失败时立即 abort 并打印文件行号。
- 性能测量必须包含 warmup（至少 3 次），计时窗口用 `cudaEventRecord` 而非 `std::chrono`。
- Stage 2 的算子题：优先使用 `<cuda/ptx>` 命名空间下的 libcu++ PTX 绑定，而非手写 inline PTX 字符串；只在 libcu++ 未覆盖的 Blackwell 新指令时降级到 inline PTX。

## 一个非常重要的现实提醒

这套文档在撰写时对应 CUDA Toolkit 13.x 与以下库版本，使用时请注意以下版本断层：

- **CUTLASS 2.x vs 3.x**：API 完全不兼容。本套文档全部使用 CUTLASS 3.x 的 `CollectiveBuilder` + `GemmUniversalAdapter` 入口。如果你查到的示例用的是 `cutlass::gemm::device::Gemm<...>` 模板，那是 2.x API，不要混用。
- **cuDNN v8 legacy vs v9 graph**：模块 J 的 cuDNN 题目全部使用 v9 的 graph API（`cudnnBackendCreateDescriptor` + operation graph 构建流程）。v8 的 `cudnnConvolutionForward` 等函数式 API 仍可用，但 v9 graph API 才能做 fused attention 等融合算子。
- **FlashAttention v2 vs v3**：FA-v2 基于 warp-level pipeline（sm_80/86），FA-v3 利用 Hopper wgmma + TMA + warp specialization。结课项目 2(b) 要求理解两者思想差异，不要用 v2 的实现去解释 v3 的性能。
- **FP8 Hopper vs Blackwell MX**：Hopper (sm_89+) 的 FP8 是 E4M3/E5M2 + per-tensor/per-block scaling；Blackwell 的 MXFP8/FP4 是 microscaling（per-group scale），指令和 quantization 语义均不同。两者不能互换。

## 推荐做题方法

每题都按下面的顺序推进：

1. 先用一句话写出你认为这题在训练什么硬件行为或设计点。
2. 先画 timeline 草图：kernel 在哪个 stream 上，数据在哪个时刻从 host 搬到 device，warp 流水如何展开。
3. 先做"必做任务"，把 `// TODO [必做]` 全部填完并跑通，再看"进阶任务"。
4. 跑通后立刻用 Nsight Compute（或 Nsight Systems）抓一次 profile，记录关键指标到 README.md 的"观察点"里。
5. 回答复盘问题，不能回答的部分标记为"待查"，下一道题之前补齐。
6. 每做完一个模块，回去重读一次 `01-心智模型.md`，看自己对 SIMT / 内存层级 / 占用率的理解是否有变化。

## 做完整套之后你应该达到什么水平

### 第一阶段完成后

- 解释为什么 warp 是 SM 调度的最小单位，以及 warp 分歧（divergence）的代价。
- 解释合并访问（coalescing）和 bank conflict 分别发生在哪一级内存，以及如何通过 Nsight Compute 的指标确认。
- 解释 `__syncthreads` 和 warp-level `__syncwarp` 的语义差异，以及什么时候用错会导致 race。
- 解释 CUDA Graph 相比 stream 的 kernel replay 有多少 launch overhead 节省，以及适用场景。
- 能独立完成 reduce / scan / histogram 三大基础算子，并在 Nsight Compute 上指出各自的瓶颈位置。

### 第二阶段完成后

- 解释 Hopper wgmma 相比 Ampere mma.sync 的执行模型差异，以及为什么 warp specialization 在 Hopper 上更重要。
- 解释 TMA 如何把数据搬运从 SM 卸载到硬件单元，以及 mbarrier transaction count 的工作机制。
- 实现一个可测量的分块 GEMM，能在 Nsight Compute 的 roofline 图上定位到正确象限并给出提升路径。
- 使用 CUTLASS 3.x 的 CollectiveBuilder 搭建一个 Hopper GEMM，理解五层 hierarchy 的职责划分。
- 手写 online softmax 和 layernorm，能解释 numerically stable 实现的 Welford 推导。
- 阅读 FlashAttention-2 源码，能指出 tiling 策略、softmax scaling、causal mask 的实现位置。
- 搭建一个 OptiX 8 Cornell Box 场景，理解 SBT 布局和 raygen / closest-hit / miss 的调用关系。

## 参考资料入口

做题过程中，建议反复对照下面这些资料的"概念定位"，而不是一上来通读全文：

### CUDA 核心

- CUDA C++ Programming Guide: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- CUDA Runtime API: https://docs.nvidia.com/cuda/cuda-runtime-api/
- CUDA C++ Best Practices Guide: https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/
- PTX ISA Reference: https://docs.nvidia.com/cuda/parallel-thread-execution/
- cuda-samples: https://github.com/NVIDIA/cuda-samples
- cccl (Thrust / CUB / libcu++): https://github.com/NVIDIA/cccl

### 架构特性

- Hopper Tuning Guide: https://docs.nvidia.com/cuda/hopper-tuning-guide/
- Ada Compatibility Guide: https://docs.nvidia.com/cuda/ada-compatibility-guide/
- Blackwell Tuning Guide: https://docs.nvidia.com/cuda/blackwell-tuning-guide/
- CUDA Features Archive: https://docs.nvidia.com/cuda/cuda-features-archive/

### Nsight 工具链

- Nsight Systems: https://docs.nvidia.com/nsight-systems/
- Nsight Compute: https://docs.nvidia.com/nsight-compute/
- Nsight Visual Studio Edition: https://docs.nvidia.com/nsight-visual-studio-edition/
- compute-sanitizer: https://docs.nvidia.com/cuda/compute-sanitizer/

### 算子与 AI Infra

- cuBLAS: https://docs.nvidia.com/cuda/cublas/
- cuDNN: https://docs.nvidia.com/deeplearning/cudnn/
- CUTLASS 3.x (含 cuTe): https://github.com/NVIDIA/cutlass
- NCCL: https://github.com/NVIDIA/nccl
- Triton: https://github.com/triton-lang/triton
- TensorRT: https://docs.nvidia.com/deeplearning/tensorrt/
- TransformerEngine: https://github.com/NVIDIA/TransformerEngine
- FlashAttention: https://github.com/Dao-AILab/flash-attention

### OptiX

- OptiX 8 Programming Guide: https://raytracing-docs.nvidia.com/optix8/guide/
- OptiX Toolkit: https://github.com/NVIDIA/optix-toolkit

## 最后一句提醒

不要把这套练习当成"我要赶快跑通多少个 kernel"。

把它当成三层训练：第一层是使用层训练——你在学习如何把计算任务映射到 GPU 线程层级并通过内存层级高效搬运数据。第二层是设计层训练——你在学习如何用 CUTLASS collective、cuTe layout、warp specialization 构建接近峰值吞吐的算子。第三层是源码层训练——你在学习从 FlashAttention / CUTLASS / TransformerEngine 的实现里读出设计决策，并能在自己的代码里复现核心思想。

三层都练透，你对 GPU 计算的理解就不再停留在"能用"，而是到达"能设计、能优化、能读源码"。

## C04 泛型与编译期桥接

[进入C04课程](../C04_Generic_CompileTime_Reflection/README.md)。CUTLASS/cuTe的类型级布局、NTTP与特化可回访C04；C04的编译成本测量不替代本课设备性能与硬件验证。本次仅增加阅读桥接，不改变CUDA源码、依赖或已有实验证据。
