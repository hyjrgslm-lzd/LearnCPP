# 模块 J · 生态库与 AI Infra

## 模块目标

这个模块是从"手写 kernel"过渡到"用生态库"的关键。你将学会 cuBLAS/cuBLASLt、cuDNN v9、NCCL、Triton、TensorRT、TransformerEngine 六大库的核心 API。不是"记住函数签名"，而是理解每个库的设计哲学：为什么提供这个 API、什么时候用、精度/性能/易用性的权衡。与模块 I 对比，看生态库的融合机制如何超越手写。

## 前置知识

- 完成模块 I（AI 算子手写实战，理解 online softmax、GEMM、FP8）
- 理解 CUTLASS/cuTe 的五层 hierarchy（模块 H）
- 熟悉 Nsight Compute 性能分析

## 模块完成标准

- 能用 cuBLAS 的 `cublasSgemm` 和 cuBLASLt 的 `cublasLtMatmul` 调用基础和高级 GEMM
- 理解 epilogue fusion 的机制（bias、GELU、scale），知道何时使用
- 掌握 cuDNN v9 graph API 的四步骤（构造、优化、获取、执行），能写 fused attention
- 理解 NCCL allreduce、allgather 的 ring vs tree 算法，会用拓扑检测
- 能写 Triton 的 `@triton.jit` kernel，编译到 PTX，与手写 CUDA 对比
- 理解 TensorRT 的 INT8/FP8 calibration 和 plugin 机制
- 掌握 TransformerEngine 的 FP8 delayed scaling 和 amax history 管理

## 硬件与工具链要求

- Compute Capability：sm_89+（FP8）；sm_90a 推荐（cuDNN v9、TE 优化）
- CUDA Toolkit：13.x+
- cuBLAS / cuBLASLt / cuDNN：CUDA Toolkit 随附
- NCCL：CUDA Toolkit 随附或单独安装
- Triton：Python 3.10+ + `pip install triton`（需编译 LLVM backend，依赖 14.x+）
- TensorRT：下载 https://developer.nvidia.com/tensorrt（需注册 NVIDIA 账户）
- TransformerEngine：`pip install transformer-engine`（或从源码编译）
- Nsight Compute：必需

---

## 练习 J1：cublas_and_cublaslt

### 目标

学会 cuBLAS 的传统 API（`cublasSgemm`、`cublasHgemm`）和 cuBLASLt 的新 API（`cublasLtMatmul` + preference + epilogue）。理解为什么 cuBLASLt 更灵活，何时选择哪个。用两个库实现同一个 GEMM，对比输出和性能。

### 前置理解

- cuBLAS：NVIDIA 维护的标准 BLAS 库，API 稳定但灵活性受限
- cuBLASLt：轻量化的新 API，支持更多 epilogue fusion（bias、GELU、scale）
- preference：调用参数优化器，告诉 cuBLASLt 目标硬件和性能优先级
- epilogue：在 GEMM 输出后直接应用后处理操作（fusion），避免回写和再读

### 必做任务

1. // TODO [必做] 构造 A、B、C 矩阵（shape: `[M, K]`、`[K, N]`、`[M, N]`），FP32 初始化。
2. // TODO [必做] 实现 cuBLAS 版本：
   - 创建 handle：`cublasCreate(&handle)`
   - 调用 `cublasSgemm`（对于 FP32）
   - 销毁 handle
   - 验证输出正确性
3. // TODO [必做] 实现 cuBLASLt 版本：
   - 创建 handle：`cublasLtCreate(&handle)`
   - 创建 matmul desc：`cublasLtMatmulDescCreate(...)`
   - 创建 layout desc（A、B、C）：`cublasLtMatrixLayoutCreate(...)`
   - 获取最优算法：`cublasLtMatmulAlgoGetHeuristic(...)`
   - 执行 matmul：`cublasLtMatmul(...)`
4. // TODO [必做] 在 cuBLASLt 版本中添加 epilogue fusion（例如 bias add + scale）。
5. // TODO [必做] 对比输出：两个版本的结果应一致（误差 < 1e-5 FP32）。
6. // TODO [必做] 用 Nsight Compute 测量：两个版本的吞吐，对比差异。

### 进阶任务

- 实现 FP8 GEMM 在 cuBLASLt 上（sm_89+，需要 FP8 input/output desc）
- 尝试多个 epilogue 组合（bias + scale + GELU），观察融合的收益
- 实现 batch GEMM（`cublasLtBatchedMatmul`）

### 验收点

- cuBLAS 和 cuBLASLt 的输出一致（误差 < 1e-5）
- epilogue fusion 的结果与分离操作一致
- 吞吐接近 GPU 峰值带宽的 70% 以上（对于大矩阵）
- 代码无内存泄漏（CUDA profiler 检查）

### 观察点

- cuBLASLt 的 preference 会自动遍历多个实现，选择最优的
- epilogue 融合减少的主要是 global memory traffic（读 C、写结果）
- 不同矩阵大小、精度、硬件上，最优算法不同（这是 preference 存在的理由）
- cuBLAS 的 API 更简洁，但 cuBLASLt 的灵活性在生产场景更重要

### 常见坑

- cuBLASLt handle 的生命周期管理（不能提前销毁）
- matmul desc 和 layout desc 的精度指定错误（e.g., 声明为 FP32 但传 FP16 数据）
- 忘记设置 epilogue 的参数（e.g., bias 指针、scale 值）
- 在 GPU 内存不足时，preference 可能选择不了最优算法，需要 fallback
- matrix layout 的 LDM（leading dimension）设置不对，导致 cache miss 或错误结果
- 多 stream 情况下，preference object 的复用问题（通常不能跨 stream）
- 没有考虑数据布局（row-major vs col-major），导致转置或错位

### 提示

- cuBLAS 调用：`cublasSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, N, M, K, &alpha, d_B, N, d_A, K, &beta, d_C, N)`
- cuBLASLt 获取算法：`cublasLtMatmulAlgoGetHeuristic(ltHandle, operationDesc, Adesc, Bdesc, Cdesc, Ddesc, preference, 1, &heuristicResult, workspaceSize)`
- epilogue 常见类型：`CUBLASLT_EPILOGUE_DEFAULT`（无）、`CUBLASLT_EPILOGUE_BIAS`（加 bias）、`CUBLASLT_EPILOGUE_BIAS_GELU`（bias + GELU）、`CUBLASLT_EPILOGUE_BGRAD_BIAS_GELU`（反向梯度）
- 验证：小矩阵对比手写 CPU GEMM

### 复盘问题

- cuBLAS 和 cuBLASLt 的 API 设计哲学差异是什么？
- epilogue fusion 减少了哪些 memory access（读/写的是什么）？
- preference 如何决定最优算法，是否总是确定的？
- 如何在 cuBLASLt 中支持新的 epilogue 类型（e.g., 自定义后处理）？
- batch GEMM 与多个单独 GEMM 的性能差异？
- FP8 在 cuBLASLt 中的 scale 管理如何工作？

### 对应官方参考

- cuBLAS Documentation: https://docs.nvidia.com/cuda/cublas/
- cuBLASLt Developer Guide: https://docs.nvidia.com/cuda/cublas/
- cuBLASLt Epilogue Fusion: https://docs.nvidia.com/cuda/cublas/
- cuda-samples: https://github.com/NVIDIA/cuda-samples （搜索 cublas 示例）

---

## 练习 J2：cudnn_v9_graph_api

### 目标

学会 cuDNN v9 graph API（取代 v8 的 legacy backend）。构造 Operation Graph、生成执行计划、执行。用 fused attention 作示例，对标模块 I4/I5 的手写版本，看库的融合如何简化代码。

### 前置理解

- cuDNN v9：NVIDIA 深度学习库的新版本，重新设计了 API
- Operation Graph：DAG 表示计算（节点是操作，边是数据依赖）
- Execution Plan：图的编译产物，包含算法选择和内存分配
- Frontend API：高级 C++ API，定义操作更直观

### 必做任务

1. // TODO [必做] 构造 Q、K、V、attention output（shape: `[batch, seq_len, d_k]` 等）。
2. // TODO [必做] 用 cuDNN frontend API 构造一个 Operation Graph，包含：
   - MatMul：Q · K^T
   - Softmax：应用到上一步输出
   - MatMul：softmax output · V
3. // TODO [必做] 编译图：调用 `cudnn_frontend::ExecutionPlan` 选择实现。
4. // TODO [必做] 执行图：填充 variant pack、调用执行。
5. // TODO [必做] 对比输出：cuDNN graph attention vs 模块 I 手写版本（unfused 或 fused）。
6. // TODO [必做] 用 Nsight Compute 测量：cuDNN graph 的吞吐与手写版本的对比。

### 进阶任务

- 在 graph 中加入 bias、GELU epilogue
- 支持 causal mask（通过 mask tensor 节点）
- 尝试 cuDNN 的其他融合操作（normalization、GEMM + activation）

### 验收点

- cuDNN graph attention 的输出与手写版本一致（误差 < 1e-3 FP16）
- graph 编译通过，无形状或类型错误
- 吞吐与 cuBLASLt 接近或更优
- 代码相对手写简洁 ≥ 30%

### 观察点

- cuDNN 的 graph 定义更声明式（描述计算，而不是过程）
- backend_operation_graph 节点的拓扑顺序很重要（不对会导致 finalize 失败）
- cuDNN 的 bias、mask 支持通过额外的 tensor 节点表示
- 为什么库的融合比手写更容易维护（bug 修复、新硬件支持）

### 常见坑

- operation 的输入/输出 tensor 尺寸声明错误
- backend_operation_graph finalize 时，节点方向或连接不对导致失败
- stride 或 layout 设置不对（NHWC vs NCHW，虽然 attention 通常 NHWC）
- mask tensor 的类型应是 INT8 或 BOOL（不能是 FP32）
- 没有为 graph 分配足够的 workspace（某些操作需要）
- variant pack 中的 device pointer 没有对齐（某些操作要求 16B 对齐）
- 多 stream 执行时，graph 的同步点管理不清

### 提示

- cuDNN frontend 的基本框架：
  ```cpp
  auto graph = cudnn_frontend::ManagedOp_v9()
      .inputs({q, k, v})
      .outputs({attention_output})
      .build();
  auto plan = graph->getExecutionPlan(handle);
  plan->execute(handle, variantPack);
  ```
- operation 常见类型：MatMul、Softmax、Norm、Pointwise（activation）、Reduction
- 验证：小 batch 对比 PyTorch 的 torch.nn.MultiheadAttention

### 复盘问题

- cuDNN v9 graph API 相对 v8 legacy backend 的改进是什么？
- 为什么 operation graph 的节点连接顺序重要？
- 如何在 graph 中表示 causal mask（hint：mask tensor + broadcast）？
- cuDNN 的融合与手写 kernel 的融合在编译时的区别？
- 如何调试 cuDNN graph 的执行错误（e.g., finalize 失败）？
- 多 GPU 情况下 graph 的复用策略？

### 对应官方参考

- cuDNN v9 Documentation: https://docs.nvidia.com/deeplearning/cudnn/
- cuDNN Frontend API: https://github.com/NVIDIA/cudnn-frontend
- cuDNN Graph Examples: https://github.com/NVIDIA/cudnn-frontend/tree/main/samples
- NVIDIA Blog: "Accelerating Deep Learning with cuDNN v9"

---

## 练习 J3：nccl_allreduce_and_topology

### 目标

学会 NCCL（NVIDIA Collective Communications Library）的基础 API：`ncclCommInitRank`、`ncclAllReduce`、`ncclAllGather`。理解 ring vs tree 算法的选择，以及 NVLink 拓扑检测。在多 GPU 单机环境下验证 allreduce 的正确性和性能。

### 前置理解

- NCCL：GPU 间通信库，提供 collectives（all-reduce、all-gather、broadcast 等）
- ring 算法：线性传递，N-1 步，适合低延迟网络（NVLink）
- tree 算法：二叉树聚合，log(N) 步，适合高延迟网络（Ethernet）
- 拓扑检测：NCCL 自动选择最优算法，但也可手动指定

### 必做任务

1. // TODO [必做] 在单机多 GPU 环境下（e.g., 4 个 GPU），初始化 NCCL communicator：
   - 调用 `ncclCommInitRank(&comm, nDev, uniqueId, myRank)`
   - 获取唯一 ID：`ncclGetUniqueId(&uniqueId)`
2. // TODO [必做] 实现 allreduce：
   - 每个 GPU 上构造一个输入向量
   - 调用 `ncclAllReduce(sendbuff, recvbuff, count, ncclFloat, ncclSum, comm, stream)`
   - 验证所有 GPU 上的输出一致
3. // TODO [必做] 实现 allgather：
   - 每个 GPU 上的向量不同
   - 调用 `ncclAllGather(sendbuff, recvbuff, count, ncclFloat, comm, stream)`
   - 验证每个 GPU 上的输出包含所有输入的连接
4. // TODO [必做] 观察 ring vs tree 算法的选择（通过 NCCL_DEBUG=INFO 日志）。
5. // TODO [必做] 检测 NVLink 拓扑（可选，用 nvidia-smi 或 NCCL 内部 API）。
6. // TODO [必做] 用多个 stream 测试 NCCL 操作的并发性。

### 进阶任务

- 实现 reduce-scatter 和 all-to-all
- 测试多机场景（需要配置 `NCCL_SOCKET_IFNAME`）
- 实现自定义的 reduce op（用户定义的操作函数）

### 验收点

- allreduce 和 allgather 的输出正确（所有 GPU 上的结果一致或正确汇聚）
- 性能：allreduce 的带宽接近 GPU 间通道容量（NVLink 下可达 50GB/s/GPU）
- 支持多 stream 并发执行
- 日志显示选择了预期的算法（ring for NVLink，tree for Ethernet）

### 观察点

- allreduce 的性能依赖 GPU 间的物理拓扑（NVLink 远快于 PCIe）
- ring 算法在高同步成本下（多 GPU 通信延迟累积）反而可能慢于 tree
- NCCL 的算法选择是自适应的（可通过环境变量覆盖）
- allreduce 与 allgather 的数据量和阶段数不同（影响性能曲线）

### 常见坑

- rank 的初始化顺序不对（所有 rank 必须一致地使用相同的 uniqueId）
- stream 同步不当（调用 NCCL op 前，stream 要保证之前的计算完成）
- communicator 没有释放（`ncclCommDestroy`），导致资源泄漏
- 在没有 NVIDIA_VISIBLE_DEVICES 限制的情况下，多进程抢占 GPU
- 数据类型指定错误（e.g., 声明为 ncclFloat 但实际是 FP16）
- ring 算法下节点故障会导致全部 hang（tree 更容错）
- 没有考虑到 allgather 输出缓冲区大小（N 倍输入大小）

### 提示

- NCCL 基础框架：
  ```cpp
  ncclUniqueId uniqueId;
  ncclComm_t comm;
  ncclGetUniqueId(&uniqueId);
  ncclCommInitRank(&comm, nDev, uniqueId, rank);
  ncclAllReduce(send, recv, count, ncclFloat, ncclSum, comm, stream);
  cudaStreamSynchronize(stream);
  ncclCommDestroy(comm);
  ```
- 设置 `NCCL_DEBUG=INFO` 查看算法选择日志
- 用 `nvidia-smi nvlink -s` 检查 NVLink 拓扑
- 验证：对比 CPU 的 allreduce 结果和 NCCL 的输出

### 复盘问题

- ring 和 tree 算法各自的优缺点是什么？
- 为什么 NVLink 下 ring 更优，Ethernet 下 tree 更优？
- 如何在多进程环境下正确初始化 NCCL communicator？
- allreduce 与 allgather 的数据流向分别是什么？
- 如何通过 Nsight Systems 观察 NCCL 操作的时间线？
- 多机分布式训练中，`NCCL_SOCKET_IFNAME` 的作用？

### 对应官方参考

- NCCL GitHub: https://github.com/NVIDIA/nccl
- NCCL Documentation: https://docs.nvidia.com/deeplearning/nccl/userguide/
- NCCL Examples: https://github.com/NVIDIA/nccl-tests
- NVIDIA Blog: "Optimizing Distributed DL Training with NCCL"

---

## 练习 J4：triton_dsl_intro

### 目标

学会 Triton Python DSL 的基础：`@triton.jit` kernel、`tl.load`/`tl.store`/`tl.dot`。写两个简单 kernel（vector add 和 naive matmul），看生成的 PTX，与手写 CUDA 对比。理解 Triton 的类型系统和自动编译流程。

### 前置理解

- Triton：Python DSL for GPU kernels，自动生成 PTX/SASS（不需要手写 `<<<>>>`）
- 程序综合（Program Synthesis）：Triton 从 Python 代码自动推导 thread block 大小、smem 分配
- 类型系统：Triton tensor type（`tl.float32[:]`）与 CUDA scalar/vector 的对应关系
- JIT 编译：`@triton.jit` 装饰器实现 just-in-time 编译到 PTX

### 必做任务

1. // TODO [必做] 安装 Triton：`pip install triton`（需要 Python 3.10+，LLVM 14.x+）。
2. // TODO [必做] 写 Triton vector-add kernel：
   - 用 `tl.arange` 生成线程索引
   - 用 `tl.load` 读取两个输入向量
   - 用 `tl.store` 写出结果
3. // TODO [必做] 写 Triton naive matmul kernel（M × K · K × N → M × N）：
   - 外层循环遍历 K（reduction）
   - 用 `tl.dot` 计算向量乘积，逐步累加
4. // TODO [必做] 生成并查看 PTX：使用 `triton.testing.assert_no_crash` 或直接 `kernel[grid](...)` 调用，查看 `.triton` 目录下的 PTX 文件。
5. // TODO [必做] 对比：将同一个 matmul 用手写 CUDA 实现，对比 PTX 指令数、寄存器占用。
6. // TODO [必做] 验证正确性：输出与 PyTorch 或 NumPy 的结果一致。

### 进阶任务

- 实现 Triton 版 softmax（参考模块 I1）
- 实现 Triton 版 LayerNorm
- 编写自定义 reduction（用 `tl.reduce`）

### 验收点

- Triton vector-add 输出正确
- Triton matmul 输出与 NumPy/PyTorch 一致（误差 < 1e-5）
- PTX 生成通过，可查看
- 与手写 CUDA 的指令数相近（可能因优化策略略有差异）

### 观察点

- Triton 生成的代码可读性比手写 PTX 高
- `tl.dot` 自动利用 Tensor Core（当底层支持时）
- Triton 的自动调优（block 大小、tile 大小选择）让代码更简洁
- Triton 的类型系统（tensor vs scalar）与 CUDA 向量化的概念类似

### 常见坑

- Triton 要求 Python 3.10+ 和 LLVM 14.x+（安装时可能需要编译）
- `tl.load` 的 mask 参数容易用错（mask=None vs mask=valid_mask）
- `tl.dot` 的 allow_tf32 参数影响精度（对标 CUDA 的 TF32 模式）
- block size 选择不当（过大导致 OOM，过小导致低效）
- 没有考虑到输入矩阵 shape 不是 block size 倍数的边界情况
- Triton 代码里的 Python 循环可能无法 JIT 编译（需要 `@triton.jit` 标注）
- 在 GPU 内存不足时，Triton 的 autotune 可能失败

### 提示

- Triton vector-add 框架：
  ```python
  @triton.jit
  def add_kernel(x_ptr, y_ptr, z_ptr, n_elements, BLOCK_SIZE: tl.constexpr):
      pid = tl.program_id(0)
      offset = pid * BLOCK_SIZE + tl.arange(0, BLOCK_SIZE)
      mask = offset < n_elements
      x = tl.load(x_ptr + offset, mask=mask)
      y = tl.load(y_ptr + offset, mask=mask)
      z = x + y
      tl.store(z_ptr + offset, z, mask=mask)
  ```
- matmul 使用 `tl.dot` 做矩阵乘法（自动用 Tensor Core）
- 查看 PTX：编译后的文件在 `~/.triton/cache/` 或通过 `triton.code_gen` 接口
- 验证：用 `torch.testing.assert_close` 比较输出

### 复盘问题

- Triton 的 `@triton.jit` 和 CUDA 的 `__global__` 分别扮演什么角色？
- `tl.load` 的 mask 参数如何处理边界情况（shape 不是倍数）？
- `tl.dot` 相对标量乘积的性能优势来自哪里（Tensor Core）？
- Triton 的类型系统（tensor vs scalar）与 CUDA 的向量化有什么对应关系？
- 如何用 Triton 的 autotune 自动选择最优 block size？
- 对比手写 CUDA 与 Triton 生成代码的可维护性？

### 对应官方参考

- Triton GitHub: https://github.com/triton-lang/triton
- Triton Documentation: https://triton-lang.org/main/
- Triton Tutorials: https://github.com/triton-lang/triton/tree/main/python/tutorials
- NVIDIA Blog: "Triton: A Python DSL for GPU Programming"

---

## 练习 J5：tensorrt_engine_build

### 目标

学会 TensorRT 的核心工作流：ONNX 模型输入、engine 构建、INT8/FP8 量化、plugin 机制。构建一个推理 engine，对标生产推理框架。理解 TensorRT 与 TE（训练）的区别（TRT 纯推理、优化编译、量化）。

### 前置理解

- TensorRT：NVIDIA 推理优化平台，将训练模型编译到优化的 GPU 代码
- ONNX：开放模型格式，TensorRT 的一个常见输入
- INT8/FP8 量化：推理时用低精度，加速计算
- plugin：自定义操作（当 TensorRT 内置操作不满足时）
- workspace：执行时需要的临时显存（layer 间数据）

### 必做任务

1. // TODO [必做] 下载并安装 TensorRT（从 https://developer.nvidia.com/tensorrt，需注册）。
2. // TODO [必做] 从 ONNX 模型加载（或用 PyTorch 导出一个简单 MLP 到 ONNX）。
3. // TODO [必做] 构建 builder 和 network definition：
   - `IBuilder* builder = createInferBuilder(logger)`
   - 从 ONNX 解析：`INetworkDefinition* network = parser->parse(...)`
4. // TODO [必做] 配置 builder options（precision、workspace 大小、batch size）。
5. // TODO [必做] 构建 INT8 engine（需要 calibration dataset）：
   - 创建 calibrator：`IInt8Calibrator* calibrator = new MyCalibrator(...)`
   - 设置 `config->setFlag(BuilderFlag::kINT8)`
   - `IHostMemory* engine_data = builder->buildSerializedNetwork(...)`
6. // TODO [必做] 执行推理：创建 context，填充输入，执行，对比输出与 FP32 baseline。

### 进阶任务

- 实现 FP8 量化（Blackwell，sm_120+）
- 写一个自定义 plugin（例如 custom activation）
- 测试不同 batch size 的推理延迟

### 验收点

- INT8 engine 构建成功，无编译错误
- INT8 推理输出与 FP32 baseline 的相对误差 < 5%（可接受范围）
- 推理延迟相对 FP32 降低 ≥ 2 倍
- plugin 正确集成到 engine 中

### 观察点

- INT8 量化的精度-性能权衡（精度损失但速度快）
- calibration 数据分布对量化精度的影响（离线 calibration 的限制）
- plugin 的自定义可以处理 TensorRT 没有的操作
- TensorRT 的编译通常比 ONNX Runtime 或 TVM 更优化（专用于 NVIDIA 硬件）

### 常见坑

- TensorRT 版本与 CUDA / cuDNN 版本不匹配（需要检查兼容性矩阵）
- INT8 calibration dataset 分布偏离线上数据，导致精度差异
- workspace 大小不足，导致某些 layer 选择更低效的算法
- plugin 的数据类型转换错误（FP32 vs FP16 vs INT8）
- 没有正确配置 batch size 和 max batch size（影响 engine 的优化）
- ONNX 模型的 opset version 与 TensorRT 支持的版本不匹配
- 在没有 NVIDIA_VISIBLE_DEVICES 的环境下，TensorRT 可能选错 GPU

### 提示

- TensorRT 基础框架（Python API）：
  ```python
  import tensorrt as trt
  logger = trt.Logger(trt.Logger.WARNING)
  builder = trt.Builder(logger)
  network = builder.create_network()
  parser = trt.OnnxParser(network, logger)
  parser.parse_from_file("model.onnx")
  config = builder.create_builder_config()
  config.set_flag(trt.BuilderFlag.INT8)
  engine = builder.build_engine(network, config)
  context = engine.create_execution_context()
  ```
- 验证：对比小 batch 推理的输出与 PyTorch

### 复盘问题

- TensorRT 与 ONNX Runtime / TVM 的设计哲学差异？
- INT8 calibration 的目的是什么（per-layer quantization parameters）？
- 如何通过 calibration 数据调整量化 scale（hint：percentile 方法）？
- plugin 的实现需要哪些虚函数（enqueue、execute、serialize）？
- batch size 和 max batch size 对 engine 编译的影响？
- 如何测试 TensorRT engine 的推理延迟（需要多次 warmup）？

### 对应官方参考

- TensorRT Documentation: https://docs.nvidia.com/deeplearning/tensorrt/
- TensorRT GitHub: https://github.com/NVIDIA/TensorRT
- TensorRT Developer Guide: https://docs.nvidia.com/deeplearning/tensorrt/developer-guide/
- NVIDIA Blog: "Deploying Models with TensorRT"

---

## 练习 J6：transformer_engine_fp8

### 目标

学会 TransformerEngine（TE）的 Python API：`te.Linear`、`te.MultiheadAttention`、`te.LayerNormLinear`。理解 FP8 delayed scaling 与 just-in-time scaling 的区别。对标模块 I 的手写算子和模块 J 前面的库，看 TE 如何集成所有优化。

### 前置理解

- TransformerEngine：NVIDIA 的 Transformer 训练库，FP8 优化、融合操作
- FP8 delayed scaling：积累若干步的 amax（绝对最大值），周期性更新 scale（避免频繁量化参数更新）
- just-in-time scaling：每步立即更新 scale（精度更好但开销大）
- amax history：维护过去 N 步的 amax，用于 scale 的稳定估计

### 必做任务

1. // TODO [必做] 安装 TransformerEngine：`pip install transformer-engine`（需要编译，可能耗时）。
2. // TODO [必做] 创建一个简单 MLP（2 层线性层）：
   - 使用 `te.Linear` 代替 `torch.nn.Linear`
   - 设置 FP8 开关：`fp8 = True`
3. // TODO [必做] 创建一个 multi-head attention：
   - 使用 `te.MultiheadAttention`
   - 配置 FP8 参数
4. // TODO [必做] 实现前向和反向传播（简单的 toy 任务）：
   - 用 TE 的 `fp8_model_init` 初始化 FP8 state
   - 观察 amax history 如何更新
5. // TODO [必做] 对比输出：TE FP8 vs PyTorch FP32 的精度漂移（应 < 1%）。
6. // TODO [必做] 用 Nsight Systems 观察：TE 的融合操作在 timeline 上如何表现（相对多个分离操作）。

### 进阶任务

- 实现 gradient accumulation + delayed scaling 的完整训练循环
- 对比 just-in-time 与 delayed scaling 的精度和性能
- 在多 GPU 环境下测试 TE（需要 NCCL）

### 验收点

- TE MLP 的 FP8 计算通过，无精度溢出
- amax history 正确维护（可通过日志检查）
- 训练一个 mini epoch，loss 下降正常（不是 nan/inf）
- 性能相对 PyTorch FP32 ≥ 1.5 倍

### 观察点

- FP8 delayed scaling 比 just-in-time 更稳定（amax 不会剧烈波动）
- amax history 窗口大小与收敛稳定性的权衡
- TE 的融合操作数量与 PyTorch 分离操作的对比（timeline 上更紧凑）
- 梯度在 FP8 中的量化与反向传播的精度影响

### 常见坑

- TE 要求特定版本的 PyTorch、CUDA、cuDNN（版本不匹配导致编译失败）
- FP8 的 amax 如果初始化为 0，会导致 nan 或 inf（需要适当初始化）
- delayed scaling 的 update_freq 选择过小（频繁更新）或过大（scale 陈旧）
- 在分布式训练中，amax history 的同步问题（不同 rank 的 amax 可能不一致）
- 混合精度训练时，某些操作不支持 FP8（e.g., dropout、normalization）
- 没有正确处理 FP8 的饱和值（某些权重接近 FP8 最大值会被截断）

### 提示

- TE 基础框架（PyTorch）：
  ```python
  import transformer_engine.pytorch as te
  model = torch.nn.Sequential(
      te.Linear(in_features, hidden_dim, fp8=True),
      torch.nn.ReLU(),
      te.Linear(hidden_dim, out_features, fp8=True)
  )
  # 初始化 FP8 state
  te.fp8_model_init(model)
  ```
- delayed scaling 参数：`te.config.update_freq_per_step`（推荐值 100-1000）
- amax history 查看：通过 TE 的日志或模块的内部状态
- 验证：对比小 batch 的梯度与 FP32 版本

### 复盘问题

- delayed scaling 为什么比 just-in-time 更稳定？
- amax history 窗口大小如何影响训练稳定性？
- TE 的融合操作相对分离操作的性能优势量化为多少？
- FP8 中的梯度反向传播如何处理精度问题？
- 多 GPU 训练时，amax 的 all-reduce 如何同步？
- 如何用 Nsight Systems timeline 观察 TE 的融合效果？

### 对应官方参考

- TransformerEngine GitHub: https://github.com/NVIDIA/TransformerEngine
- TE Documentation: https://docs.nvidia.com/deeplearning/transformer-engine/user-guide/
- TE FP8 Guide: https://docs.nvidia.com/deeplearning/transformer-engine/user-guide/
- NVIDIA Blog: "TransformerEngine: Enabling FP8 Training in Transformers"

---

## 做完本模块后应达到的水平

- 能独立调用 cuBLAS、cuBLASLt、cuDNN v9、NCCL、Triton、TensorRT、TE，理解各库的设计哲学和适用场景
- 掌握 epilogue fusion 的机制，知道何时手写 kernel vs 用库的 fusion
- 理解分布式训练的通信库（NCCL），能在多 GPU 上协调计算
- 能用 Triton 快速原型 GPU kernel（相对手写 CUDA 更高效）
- 掌握推理优化（TensorRT）的 INT8/FP8 量化和 engine 构建
- 能用 TE 进行高效的 FP8 训练，理解 delayed scaling 的稳定性
- 能用 Nsight Compute 和 Nsight Systems 对比不同库的性能（timeline 融合度、memory traffic、寄存器占用）
- 明白"手写最优"vs"库的易用性"vs"自动优化"的权衡，在实践中做出合理选择
