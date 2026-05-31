# common/ 共享头索引

本目录提供所有习题共享的辅助头文件。本文件维护 **代码注释中文对照表**，与源代码中的 `[CMN-Txx]` 标号一一对应。

代码本身保持纯 ASCII（无 BOM、无中文、无 Unicode 装饰字符），这样 nvcc 在任何 Windows 工具链组合下都能可靠编译；中文教学说明仅出现在 README 中，通过 Tag 反向定位。

## 文件清单

| 文件 | 作用 |
|---|---|
| `cuda_check.cuh` | CUDA 运行时 / 驱动 API 错误检查宏 |
| `device_info.cuh` | CUDA 设备属性查询与打印工具 |
| `timer.cuh` | CUDA 事件计时器与主机端计时器 |
| `nvtx_range.cuh` | NVTX 范围 RAII 辅助（Nsight Systems 时间线集成） |

## 代码注释中文对照（cuda_check.cuh）

- `[CMN-T01]` (cuda_check.cuh:2) `cuda_check.cuh` —— CUDA / 驱动 API 错误检查宏。
- `[CMN-T02]` (cuda_check.cuh:4) **使用场景**：
  - `CUDA_CHECK`：包裹所有 `cudaXxx()` 运行时 API 调用（`cuda_runtime.h`）。
  - `CUDA_CHECK_LAST`：kernel 启动后检查异步错误（等价 `cudaGetLastError`）。
  - `CU_CHECK`：包裹驱动 API `cuXxx()` 调用（`cuda.h` / `libcuda`）。驱动 API 更底层，常用于 TMA / cuTensorMap / module 加载；运行时 API 内部也调用驱动 API，两者不要混用同一 context。
- `[CMN-T03]` (cuda_check.cuh:16) 运行时 API 检查（最常用路径）。
- `[CMN-T04]` (cuda_check.cuh:33) kernel 启动后立即检查（捕获非阻塞启动留下的异步错误）。
- `[CMN-T05]` (cuda_check.cuh:46) 驱动 API 检查（需要 `<cuda.h>`；CUDA Toolkit 附带）。
- `[CMN-T06]` (cuda_check.cuh:47) 仅在包含 `cuda.h` 或显式定义 `CUDA_VERSION` 时启用，避免纯运行时编译单元报错。

## 代码注释中文对照（device_info.cuh）

- `[CMN-T10]` (device_info.cuh:2) `device_info.cuh` —— CUDA 设备属性查询与打印工具。
- `[CMN-T11]` (device_info.cuh:4) 提供三类接口：
  - `print_device_info()`：向 stdout 打印完整设备信息（调试 / 开机自检用）。
  - `get_peak_memory_bandwidth_gbps()`：计算理论峰值显存带宽（GB/s）。
  - `has_hopper_features()`：判断是否具备 Hopper（CC 9.0+）特性。
  - `has_blackwell_features()`：判断是否具备 Blackwell（CC 10.0+）特性。
- `[CMN-T12]` (device_info.cuh:14) 打印完整设备信息。
- `[CMN-T13]` (device_info.cuh:21) 基本标识。
- `[CMN-T14]` (device_info.cuh:24) Hopper 及以上使用 `sm_XXa` 加速架构标识。
- `[CMN-T15]` (device_info.cuh:30) 执行资源（SM / Block / Warp）。
- `[CMN-T16]` (device_info.cuh:35) 共享内存。
- `[CMN-T17]` (device_info.cuh:36) 默认值（编译期静态上限）。
- `[CMN-T18]` (device_info.cuh:42) opt-in 动态上限（`cudaFuncSetAttribute` 可提升至此值）。
- `[CMN-T19]` (device_info.cuh:51) 寄存器。
- `[CMN-T20]` (device_info.cuh:54) 显存与 L2 缓存。
- `[CMN-T21]` (device_info.cuh:60) 显存带宽（理论峰值）。
- `[CMN-T22]` (device_info.cuh:61) CUDA 13 起 `cudaDeviceProp::memoryClockRate` 被移除，改用 `cudaDeviceGetAttribute(cudaDevAttrMemoryClockRate, ...)`。
- `[CMN-T23]` (device_info.cuh:67) 带宽 (GB/s) = 时钟频率 (Hz) × 总线位宽 (bit) / 8 × 2 (DDR) / 1e9。
- `[CMN-T24]` (device_info.cuh:75) PCIe 标识。
- `[CMN-T25]` (device_info.cuh:79) Hopper+ 特性：thread-block cluster 调度。
- `[CMN-T26]` (device_info.cuh:82) `cudaDevAttrClusterLaunch` 在 CUDA 11.8+ / sm_90+ 可用。
- `[CMN-T27]` (device_info.cuh:89) 异步引擎数量。
- `[CMN-T28]` (device_info.cuh:96) 返回理论峰值全局显存带宽（GB/s，整数向下取整）。
- `[CMN-T29]` (device_info.cuh:104) 与 `print_device_info` 中计算公式保持一致。
- `[CMN-T30]` (device_info.cuh:111) Hopper（CC ≥ 9.0）特性检测。
- `[CMN-T31]` (device_info.cuh:112) 依赖：wgmma / TMA / distributed shared memory / cluster。
- `[CMN-T32]` (device_info.cuh:120) Blackwell（CC ≥ 10.0）特性检测。
- `[CMN-T33]` (device_info.cuh:121) 依赖：MXFP8 / FP4、第五代 Tensor Core、新 TMA 扩展。

## 代码注释中文对照（timer.cuh）

- `[CMN-T34]` (timer.cuh:2) `timer.cuh` —— CUDA 事件计时器与主机计时器。
- `[CMN-T35]` (timer.cuh:4) **计时策略选择**：
  - `CudaEventTimer`：设备端精确计时（GPU 硬件时间戳，排除 CPU→GPU 调度抖动）。适合测量 kernel、memcpy、stream 段落；分辨率约 0.5 µs。
  - `HostTimer`：主机端 `steady_clock`，适合端到端 wall-clock 测量，包含 kernel 启动延迟和 CPU 同步开销。
  - `ScopedCudaTimer`：RAII 包装，析构时自动打印耗时，适合快速 profiling 插桩。
- `[CMN-T36]` (timer.cuh:21) `CudaEventTimer` —— 基于 `cudaEvent` 的设备端计时器。
- `[CMN-T37]` (timer.cuh:30) 禁止拷贝（事件句柄不可复制）。
- `[CMN-T38]` (timer.cuh:39) 在指定 stream 上记录开始时间戳。
- `[CMN-T39]` (timer.cuh:44) 记录结束时间戳，并在 CPU 侧等待 stop 事件完成。
- `[CMN-T40]` (timer.cuh:50) 返回 `start→stop` 之间的毫秒数（需先调用 `stop()`）。
- `[CMN-T41]` (timer.cuh:62) `HostTimer` —— 基于 `std::chrono::steady_clock` 的主机端计时器。
- `[CMN-T42]` (timer.cuh:67) 重置起始点。
- `[CMN-T43]` (timer.cuh:70) 返回自构造（或 `reset`）以来经过的毫秒数。
- `[CMN-T44]` (timer.cuh:82) `ScopedCudaTimer` —— RAII 计时器，析构时自动打印标签和耗时。
- `[CMN-T45]` (timer.cuh:83) **用法**：在需要计时的作用域开头声明 `ScopedCudaTimer t("my_kernel");`，离开作用域后自动输出 `[my_kernel] 1.234 ms`。
- `[CMN-T46]` (timer.cuh:95) 禁止拷贝。

## 代码注释中文对照（nvtx_range.cuh）

- `[CMN-T49]` (nvtx_range.cuh:2) `nvtx_range.cuh` —— NVTX 范围标注 RAII 辅助（Nsight Systems 时间线集成）。
- `[CMN-T50]` (nvtx_range.cuh:4) **Nsight Systems 工作流**：
  1. 在代码关键区域插入 `NVTX_RANGE("label")` 或构造 `NvtxRange` 对象。
  2. 用 `nsys profile --trace=cuda,nvtx ./executable` 采集。
  3. 在 Nsight Systems GUI 的 NVTX 行查看彩色区间，与 CUDA kernel 时间线对齐。
  4. 嵌套调用会形成层级范围，适合标注 forward / backward / optimizer 等阶段。
- `[CMN-T51]` (nvtx_range.cuh:13) **禁用开关**：定义 `GPU_STUDY_DISABLE_NVTX` 可将所有宏和类编译为空操作，用于 release build 或无 Nsight 环境，零开销。
- `[CMN-T52]` (nvtx_range.cuh:18) 实现分支：启用 vs 禁用。
- `[CMN-T53]` (nvtx_range.cuh:22) NVTX3 头文件随 CUDA Toolkit 一并安装，路径 `include/nvtx3/nvToolsExt.h`。
- `[CMN-T54]` (nvtx_range.cuh:26) `NvtxRange` —— RAII 范围标注类。ctor 推入一个命名范围；dtor 弹出（无论是否异常退出作用域）。
- `[CMN-T55]` (nvtx_range.cuh:32) 仅指定名称（使用默认颜色）。
- `[CMN-T56]` (nvtx_range.cuh:37) 指定名称 + ARGB 颜色（例如 `0xFF00FF00` = 不透明绿色）。
- `[CMN-T57]` (nvtx_range.cuh:51) 禁止拷贝和移动（范围必须严格按作用域嵌套）。
- `[CMN-T58]` (nvtx_range.cuh:65) `NVTX_RANGE(name)` —— 在当前作用域内创建一个命名范围。
- `[CMN-T59]` (nvtx_range.cuh:66) 使用 `__LINE__` 确保同函数内多次使用时变量名不冲突。
- `[CMN-T60]` (nvtx_range.cuh:69) `NVTX_RANGE_COLOR(name, argb)` —— 带颜色的范围。
- `[CMN-T61]` (nvtx_range.cuh:72) `NVTX_MARK(name)` —— 点事件（无持续时间，在时间线上显示为标记）。
- `[CMN-T62]` (nvtx_range.cuh:80) 空操作结构体：编译器会将其完全优化掉。
- `[CMN-T63]` (nvtx_range.cuh:91) 空操作宏：每次调用折叠为 void 转换。
