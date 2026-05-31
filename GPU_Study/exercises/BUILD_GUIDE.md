# GPU_Study 构建指南

## 依赖清单

### 必须安装

| 依赖 | 版本要求 | 说明 |
|------|----------|------|
| CUDA Toolkit | 13.x | 包含 nvcc、cudart、Nsight Compute CLI（ncu）、compute-sanitizer |
| Visual Studio 2026 Build Tools | 最新 | 提供 MSVC 19.5x host 编译器；也可用完整 IDE 版本 |
| CMake | 3.28+ | `cmake --version` 确认；低版本不支持 `LANGUAGES CUDA` 新特性 |

### 推荐安装

| 依赖 | 说明 |
|------|------|
| Ninja | 命令行构建比 MSBuild 更快；`ninja --version` 确认 |
| Nsight Systems | `nsys` 全系统时间线剖析；随 CUDA Toolkit 安装或单独下载 |
| Nsight Compute | `ncu` kernel 级别 roofline、source-SASS 对应；随 CUDA Toolkit 安装 |
| Nsight Visual Studio Edition | VS 内核断点调试；从 NV 开发者网站单独安装 |

### 按模块安装

| 依赖 | 适用模块 | 说明 |
|------|----------|------|
| CUTLASS 3.x | 模块 H、结课 2a | **自动**通过 FetchContent 下载，无需手动安装 |
| OptiX SDK 8.x | 模块 K、结课 2c | NV 许可协议禁止 FetchContent，需从 [raytracing-docs.nvidia.com](https://raytracing-docs.nvidia.com/optix8/) 手动下载并安装；设置环境变量 `OPTIX_SDK_ROOT=<安装路径>` |
| Python 3.10+ 虚拟环境 | 模块 J（Triton 题） | `python -m venv .venv && .venv\Scripts\activate && pip install triton` |

---

## 构建方式

### 推荐：Visual Studio 2026

```bat
rem 在 exercises\ 目录下执行
cmake --preset vs2026
cmake --build build-vs2026 --config Release
```

若使用 VS 2026 IDE，直接打开 `exercises\` 文件夹，CMake 工具会自动识别 `CMakePresets.json`，
在下拉框选择 `Visual Studio 18 2026` 配置即可。

### 命令行用户（Ninja）

```bat
cmake --preset ninja-release
cmake --build build-ninja-release
```

Debug 构建：

```bat
cmake --preset ninja-debug
cmake --build build-ninja-debug
```

### VS 2022 回退

```bat
cmake --preset vs2022
cmake --build build-vs2022 --config Release
```

---

## 特定模块额外要求

### 模块 G / H（Tensor Core、TMA、CUTLASS）

- **必须**有 sm_90a（Hopper）实机运行 `wgmma.mma_async` 和 TMA 相关题目。
- Ampere/Ada（sm_80/86/89）机器可运行回退版 `wmma`、`mma.sync` 题目。
- Blackwell（sm_100a/120a）题目为进阶，非必须。

### 模块 K（OptiX 光线追踪）

1. 安装 OptiX SDK 8.x（需接受 NV 最终用户许可协议）。
2. 设置环境变量：
   ```bat
   set OPTIX_SDK_ROOT=C:\ProgramData\NVIDIA Corporation\OptiX SDK 8.x.x
   ```
3. 或在 CMake 配置时传入：
   ```bat
   cmake --preset vs2026 -DOPTIX_SDK_ROOT="C:\ProgramData\NVIDIA Corporation\OptiX SDK 8.x.x"
   ```

### 模块 J（Triton 题）

```bat
python -m venv .venv
.venv\Scripts\activate
pip install triton torch
```

Triton 在 Windows 上支持有限，建议在 WSL2 或 Linux 环境运行 Triton 题目。

---

## Nsight 工作流

### 打开 line info（Nsight Compute source-SASS 对应）

```bat
cmake --preset ncu-profile
cmake --build build-ncu-profile
```

此预设等价于 ninja-release + `GPU_STUDY_LINEINFO=ON`，对所有 `.cu` 传入 `--generate-line-info`。

### Nsight Compute 采集报告

```bat
ncu --set full -o report.ncu-rep ./build-ncu-profile/A1_hello_device/a1_hello_device.exe
```

用 Nsight Compute GUI 打开 `report.ncu-rep`，在 Source 页签查看 source-SASS 对应，
在 Roofline 页签确认带宽 / 计算利用率。

### Nsight Systems 时间线追踪

```bat
nsys profile -o trace.nsys-rep --trace=cuda,nvtx ./build-ncu-profile/E1_streams_events/e1_streams_events.exe
```

用 Nsight Systems GUI 打开 `trace.nsys-rep`，查看 kernel 并发、memcpy 与 NVTX range 对应关系。

### compute-sanitizer

```bat
rem 内存错误检测（越界、非法访问）
compute-sanitizer --tool memcheck ./build-ninja-debug/A1_hello_device/a1_hello_device.exe

rem 竞态检测（shared memory 未同步）
compute-sanitizer --tool racecheck ./build-ninja-debug/C1_cooperative_groups/c1_cooperative_groups.exe

rem 同步错误检测（__syncthreads 调用不一致）
compute-sanitizer --tool synccheck ./build-ninja-debug/C2_syncthreads_warp/c2_syncthreads_warp.exe
```

---

## 常见构建问题

### 1. CUDA host compiler 版本不匹配

**现象**：`nvcc` 报 `unsupported Microsoft Visual Studio version` 或 `host compiler version is not supported`。

**原因**：CUDA Toolkit 13.x 需要 MSVC 19.4x 或更高版本（VS 2022 17.x / VS 2026）。

**解决**：
- 确认 `cl.exe` 版本：`cl` 输出第一行包含版本号，需 ≥ 19.40。
- 若系统有多个 VS 版本，通过 `-T host=<toolset>` 或 `CMAKE_CUDA_HOST_COMPILER` 指定正确的 `cl.exe`：
  ```bat
  cmake --preset vs2026 -DCMAKE_CUDA_HOST_COMPILER="C:/Program Files/Microsoft Visual Studio/2026/Community/VC/Tools/MSVC/14.4x.xxxxx/bin/Hostx64/x64/cl.exe"
  ```

### 2. 架构未列全导致 PTX JIT 警告或运行时错误

**现象**：运行时打印 `no kernel image available for the device`，或 ncu 报告 `PTX JIT`。

**原因**：`CMAKE_CUDA_ARCHITECTURES` 未包含当前 GPU 的 compute capability。

**解决**：确认当前 GPU 架构（`deviceQuery` 或 `nvidia-smi --query-gpu=compute_cap`），
在顶层 `CMakeLists.txt` 的 `CMAKE_CUDA_ARCHITECTURES` 列表中加入对应值，
例如 Ada Lovelace 为 `89`，Hopper 为 `90a`。

### 3. CUTLASS FetchContent 克隆过大、速度慢

**现象**：`cmake --preset ...` 时长时间卡在 `Fetching CUTLASS`，甚至超时失败。

**原因**：CUTLASS 仓库历史较大。`CutlassSetup.cmake` 已设置 `GIT_SHALLOW TRUE`，
但网络状况差时仍可能超时。

**解决方案（任选其一）**：
- 使用代理或镜像：`set HTTPS_PROXY=http://127.0.0.1:<port>`
- 手动克隆后设置本地路径缓存：
  ```bat
  git clone --depth=1 https://github.com/NVIDIA/cutlass.git third_party\cutlass-src
  cmake --preset vs2026 -DFETCHCONTENT_SOURCE_DIR_CUTLASS="%CD%\third_party\cutlass-src"
  ```
- 锁定较小的 release tag 避免 main 分支体积：
  ```bat
  cmake --preset vs2026 -DGPU_STUDY_CUTLASS_TAG=v3.5.0
  ```

### 4. `/std:c++latest` 与 nvcc 兼容性问题

**现象**：nvcc 编译 `.cu` 时报 `unknown compiler option /std:c++latest` 或 `/std:c++latest is not a supported option`。

**原因**：`/std:c++latest` 是 CXX host 选项，不应直接传给 nvcc 设备编译前端。

**解决**：顶层 `CMakeLists.txt` 的 MSVC 补丁块已使用
`$<$<COMPILE_LANGUAGE:CXX>:...>` 生成器表达式限制作用范围，
只对 `.cpp` 文件生效，不影响 `.cu`。若子目录的 `CMakeLists.txt` 裸用
`target_compile_options(...  /std:c++latest)` 而不加语言限制，则会传到 nvcc，需修正。

### 5. OptiX SDK 路径未设置导致模块 K 配置失败

**现象**：`cmake --preset ...` 时报 `Could not find OptiX SDK` 或找不到 `optix.h`。

**原因**：OptiX SDK 需手动安装且路径不在标准位置。

**解决**：设置 `OPTIX_SDK_ROOT` 环境变量或 CMake 变量（见"特定模块额外要求"）。
模块 K 的 `CMakeLists.txt` 使用 `find_path(OPTIX_INCLUDE optix.h HINTS ${OPTIX_SDK_ROOT}/include)` 探测，
路径正确即可通过配置。

### 6. `CMAKE_CUDA_ARCHITECTURES` 含 `90a`/`100a`/`120a` 在旧版 CUDA 报错

**现象**：`cmake` 报 `Unknown CUDA architecture`。

**原因**：`90a`（Hopper）、`100a`（Blackwell B100）、`120a`（Blackwell B200）需 CUDA 12.0+ / 13.x 才识别。

**解决**：升级至 CUDA Toolkit 13.x；或在老机器上通过 CMake 命令行覆盖：
```bat
cmake --preset vs2022 -DCMAKE_CUDA_ARCHITECTURES="80;86;89"
```

---

## CI / Headless 场景说明

在无 GPU 的 CI 环境（如 GitHub Actions 标准 runner）：

- `cmake --preset ninja-release && cmake --build build-ninja-release` 可以**仅编译**通过，
  前提是 `CUDAToolkit` 已安装（只需 nvcc，不需驱动）。
- 运行阶段（`./exe`）会因 `cudaGetDeviceCount` 返回 0 而立即退出；
  `CUDA_CHECK` 宏会打印错误并 `abort()`，此为预期行为。
- 算子题（模块 I）的数值正确性验证、roofline 采集在无 GPU 环境**无法执行**；
  参考答案的 `reference/` 目录内含预期输出，CI 可用文本比对代替实机运行。
- Nsight Compute / Nsight Systems 在无 GPU 时无法采集，`ncu`/`nsys` 命令会报错退出；
  CI 中应跳过 Nsight 相关步骤（用 `if(CI)` CMake 变量或脚本条件判断）。
- CUTLASS FetchContent 需要网络访问；CI 中建议缓存 `exercises/third_party/` 目录（按 tag hash 做 cache key）以减少拉取时间。
