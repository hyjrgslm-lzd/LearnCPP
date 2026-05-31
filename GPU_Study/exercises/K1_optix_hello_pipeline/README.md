# K1: hello_optix_pipeline

本练习是 OptiX 8.x 模块的起点。你将从零搭建最小可运行的光线追踪管线，
输出一张 1024×1024 的纯红色图像，同时掌握 OptiX host 侧 boilerplate 的全部
关键步骤。

---

## OptiX SDK 安装

OptiX SDK 受 NVIDIA 许可协议限制，**不能通过 FetchContent 自动下载**，
必须手动安装。请按以下步骤操作：

1. **下载 SDK**
   前往 https://developer.nvidia.com/designworks/optix/download
   选择 OptiX SDK 8.x（8.0.0 或 8.1.0），下载 Windows 安装包（.exe）。
   需要登录 NVIDIA Developer 账号（免费注册）。

2. **安装 SDK**
   运行安装包，默认安装路径为：
   ```
   C:\ProgramData\NVIDIA Corporation\OptiX SDK 8.0.0\
   ```
   安装完成后确认以下文件存在：
   ```
   C:\ProgramData\NVIDIA Corporation\OptiX SDK 8.0.0\include\optix.h
   ```

3. **设置环境变量**
   在系统环境变量中新增（或修改）`OPTIX_SDK_ROOT`，值为 SDK 根目录：
   ```
   OPTIX_SDK_ROOT=C:\ProgramData\NVIDIA Corporation\OptiX SDK 8.0.0
   ```
   PowerShell 临时设置（仅当前会话有效）：
   ```powershell
   $env:OPTIX_SDK_ROOT = "C:\ProgramData\NVIDIA Corporation\OptiX SDK 8.0.0"
   ```

4. **重启 Visual Studio / CMake GUI**
   环境变量修改后需重启 IDE 或命令行窗口才能生效。

5. **重新配置 CMake**
   在 CMake GUI 或命令行执行 Configure，CMake 输出应显示：
   ```
   K1: OptiX include dir = C:/ProgramData/NVIDIA Corporation/OptiX SDK 8.0.0/include
   ```
   若仍显示 stub 提示，检查环境变量拼写和路径是否正确。

6. **验证编译**
   构建 `K1_programs_optixir` target 先行编译 `.optixir`，
   再构建 `K1_optix_hello_pipeline`。

---

## 练习目标

`[K1-T01]` (programs.cu:3) 练习 K1 OptiX 设备程序。
`[K1-T02]` (programs.cu:4) 空注释行。
`[K1-T03]` (programs.cu:5) 编译方式（由 CMakeLists.txt 中 add_custom_command 自动执行）：
`[K1-T04]` (programs.cu:6) `nvcc --optix-ir -I<OPTIX_INCLUDE_DIR> -std=c++17 -arch=sm_75`。
`[K1-T05]` (programs.cu:7) `-o programs.optixir programs.cu`。
`[K1-T06]` (programs.cu:8) 空注释行。
`[K1-T07]` (programs.cu:9) 包含的 program type：
`[K1-T08]` (programs.cu:10) `__raygen__hello` — 写出固定红色像素到 output buffer。
`[K1-T09]` (programs.cu:11) `__miss__noop` — K1 不调用 optixTrace，此程序为占位符。

`[K1-T40]` (host.cpp:3) 练习 K1：hello_optix_pipeline。
`[K1-T41]` (host.cpp:4) 空注释行。
`[K1-T42]` (host.cpp:5) 目标：最小 OptiX 8.x host 侧 boilerplate。
`[K1-T43]` (host.cpp:6) 初始化 CUDA context + OptiX device context。
`[K1-T44]` (host.cpp:7) 从 .optixir 加载 module。
`[K1-T45]` (host.cpp:8) 注册 raygen program group。
`[K1-T46]` (host.cpp:9) 构建 pipeline。
`[K1-T47]` (host.cpp:10) 构造 SBT（raygen 段）。
`[K1-T48]` (host.cpp:11) optixLaunch 生成 1024×1024 纯红色图像。
`[K1-T49]` (host.cpp:12) 将结果写出为 output_k1.ppm。
`[K1-T50]` (host.cpp:13) 空注释行。
`[K1-T51]` (host.cpp:14) 必做步骤对应模块文档 K1 第 1-10 项。

写出第一个 OptiX 8 管线：

- 初始化 CUDA driver context + OptiX device context
- 从 `.optixir` 加载 OptiX module
- 注册 raygen program group
- 构建 pipeline
- 构造 SBT（raygen 段）
- `optixLaunch` 生成图像
- 将结果读回 host 写成 PPM 文件

---

## 文件结构

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | 构建脚本，含 `.optixir` custom command |
| `host.cpp` | Host 侧驱动代码（C++26，不含 device kernel）|
| `programs.cu` | OptiX device 程序，编译为 `.optixir` |
| `README.md` | 本文档 |

---

## 必做任务清单

`[K1-T10]` (programs.cu:18) Params — 必须与 host.cpp 中定义完全一致（字段顺序、类型、对齐）。
`[K1-T11]` (programs.cu:19) 同上：字段顺序、类型、对齐保持一致。
`[K1-T12]` (programs.cu:27) OptiX 要求 launch params 通过 `__constant__` 变量传递。
`[K1-T13]` (programs.cu:30) `__raygen__hello`。
`[K1-T14]` (programs.cu:31) 空注释行。
`[K1-T15]` (programs.cu:32) 每像素执行一次；写出固定 RGBA 颜色（红色）到 output buffer。
`[K1-T16]` (programs.cu:33) 空注释行。
`[K1-T17]` (programs.cu:34) TODO [必做] 步骤 3（对应文档 K1 第 3 项）：
`[K1-T18]` (programs.cu:35) 用 `optixGetLaunchIndex()` 获取像素坐标 (x, y)。
`[K1-T19]` (programs.cu:36) 计算线性索引 idx = y * width + x。
`[K1-T20]` (programs.cu:37) 写出 `make_float4(1, 0, 0, 1)`（纯红色）。
`[K1-T21]` (programs.cu:48) TODO [必做] 写出颜色到 `params.output[y * w + x]`。
`[K1-T22]` (programs.cu:50) TODO [进阶] 把颜色改为基于坐标的渐变。
`[K1-T23]` (programs.cu:57) `__miss__noop`。
`[K1-T24]` (programs.cu:58) 空注释行。
`[K1-T25]` (programs.cu:59) K1 中 raygen 直接写 output，不调用 optixTrace，此 miss 程序为占位符。
`[K1-T26]` (programs.cu:60) 后续 K2 中将扩展为返回背景颜色。
`[K1-T27]` (programs.cu:61) 同上。
`[K1-T28]` (programs.cu:62) 空注释行。
`[K1-T29]` (programs.cu:63) TODO [进阶] 当 K1 raygen 调用 optixTrace 时，在此写入背景颜色到 payload。
`[K1-T30]` (programs.cu:64) 同上。
`[K1-T31]` (programs.cu:67) TODO [进阶] `optixSetPayload_0(__float_as_uint(0.2f)); // R`。
`[K1-T32]` (programs.cu:68) TODO [进阶] `optixSetPayload_1(__float_as_uint(0.3f)); // G`。
`[K1-T33]` (programs.cu:69) TODO [进阶] `optixSetPayload_2(__float_as_uint(0.8f)); // B`。

对应模块文档 K1 第 1-10 项：

- [ ] 步骤 1：`cuInit` + `cuDeviceGet` + `cuCtxCreate`
- [ ] 步骤 2：`optixInit()` + `optixDeviceContextCreate()` with log callback
- [ ] 步骤 3：读取 `.optixir` 文件 + `optixModuleCreate()`
- [ ] 步骤 4：`OptixProgramGroupDesc` (RAYGEN) + `optixProgramGroupCreate()`
- [ ] 步骤 5：`OptixPipelineCompileOptions` + `optixPipelineCreate()`
- [ ] 步骤 6：`cudaMalloc` output buffer（float4，1024×1024）
- [ ] 步骤 7：填充 `Params` struct + `cuMemcpyHtoD`
- [ ] 步骤 8：`optixSbtRecordPackHeader` + 上传 SBT record
- [ ] 步骤 9：`optixLaunch` + `cudaStreamSynchronize`
- [ ] 步骤 10：`cudaMemcpy` 读回 + 写出 `output_k1.ppm`

---

## host.cpp 任务标记

`[K1-T52]` (host.cpp:18) SDK 缺失时的 stub。
`[K1-T55]` (host.cpp:27) 正式实现。
`[K1-T56]` (host.cpp:29) 驱动 API — 必须在 cuda_check.cuh 之前 include 以触发 CU_CHECK 分支。
`[K1-T57]` (host.cpp:33) OptiX 8.x 头文件（header-only + 动态加载，无需链接 lib）。
`[K1-T58]` (host.cpp:35) 每个二进制文件只能出现一次。
`[K1-T59]` (host.cpp:38) 项目公共工具。
`[K1-T60]` (host.cpp:53) `OPTIX_CHECK` — 检查 OptixResult，失败时打印错误并 abort。
`[K1-T61]` (host.cpp:70) OptiX log callback（输出到 stderr；level 1=fatal 4=print）。
`[K1-T62]` (host.cpp:80) 从文件读取二进制内容（用于加载 .optixir）。
`[K1-T64]` (host.cpp:96) Params — 与 programs.cu 共享的常量内存布局。
`[K1-T65]` (host.cpp:97) device 端输出 buffer（RGBA float4）。
`[K1-T66]` (host.cpp:103) SBT record 辅助类型。
`[K1-T67]` (host.cpp:104) OptiX 要求每条 SBT record = header(32 字节) + user data。
`[K1-T68]` (host.cpp:105) 总大小 16 字节对齐。
`[K1-T69]` (host.cpp:113) K1 raygen 无额外 per-record 数据，占位符保持对齐。
`[K1-T70]` (host.cpp:122) 将 float4 输出写成 PPM P6 格式（8 位 RGB）。
`[K1-T72]` (host.cpp:142) `main`。
`[K1-T74]` (host.cpp:151) TODO [必做] 步骤 1：CUDA 驱动初始化。
`[K1-T75]` (host.cpp:152) 必须在 optixInit() 前先建立 CUDA context。
`[K1-T76]` (host.cpp:153) （OptiX 依赖驱动 API context）。
`[K1-T77]` (host.cpp:159) TODO [必做] 步骤 2：OptiX 初始化。
`[K1-T78]` (host.cpp:160) `optixInit()` 动态加载 OptiX 函数表。
`[K1-T79]` (host.cpp:161) 不需要静态链接 .lib。
`[K1-T80]` (host.cpp:163) TODO [必做] 步骤 2b：创建 OptiX device context。
`[K1-T81]` (host.cpp:168) 4 = 输出所有等级日志。
`[K1-T82]` (host.cpp:169) TODO [必做] `optixDeviceContextCreate(cu_ctx, &opts, &optix_ctx)`。
`[K1-T84]` (host.cpp:174) TODO [必做] 步骤 3：从 .optixir 文件读取设备程序。
`[K1-T85]` (host.cpp:175) `K1_OPTIXIR_PATH` 由 CMakeLists.txt 通过 `target_compile_definitions` 注入。
`[K1-T87]` (host.cpp:178) TODO [必做] 读取 `optixir_path` 文件内容到 `std::vector<char>`。
`[K1-T88]` (host.cpp:181) TODO [必做] 步骤 3b：创建 OptiX module。
`[K1-T89]` (host.cpp:192) K1 raygen 不使用 payload。
`[K1-T90]` (host.cpp:200) TODO [必做] `optixModuleCreate(...)`。
`[K1-T92]` (host.cpp:213) TODO [必做] 步骤 4：创建 raygen program group。
`[K1-T93]` (host.cpp:223) TODO [必做] `optixProgramGroupCreate(...)`。
`[K1-T95]` (host.cpp:230) TODO [必做] 步骤 5：构建 pipeline。
`[K1-T96]` (host.cpp:237) TODO [必做] `optixPipelineCreate(...)`。
`[K1-T98]` (host.cpp:249) TODO [必做] 步骤 6：分配 output buffer。
`[K1-T99]` (host.cpp:254) TODO [必做] 步骤 7：上传 Params 到设备。
`[K1-T100]` (host.cpp:264) TODO [必做] 步骤 8：构造 SBT（raygen 段）。
`[K1-T101]` (host.cpp:265) SBT record = header(32字节 program group handle) + user data。
`[K1-T102]` (host.cpp:267) TODO [必做] `optixSbtRecordPackHeader(raygen_pg, &h_rg_record)`。
`[K1-T103]` (host.cpp:276) miss / hitgroup 在 K1 中留空（不调用 optixTrace）。
`[K1-T104]` (host.cpp:280) TODO [必做] 步骤 9：启动光线追踪。
`[K1-T105]` (host.cpp:286) TODO [必做] `optixLaunch(...)`。
`[K1-T106]` (host.cpp:295) TODO [必做] 等待 GPU 完成（stream 同步）。
`[K1-T108]` (host.cpp:300) TODO [必做] 步骤 10：读回结果并写出图像。
`[K1-T109]` (host.cpp:302) TODO [必做] `cudaMemcpy(...)` device->host。
`[K1-T110]` (host.cpp:311) TODO [进阶] 把颜色改为 threadIdx/blockDim 生成的渐变。
`[K1-T111]` (host.cpp:312) TODO [进阶] 在 raygen 里调用 optixTrace 与 miss 通讯。
`[K1-T112]` (host.cpp:313) TODO [进阶] 添加 closest-hit，打到面片时返回不同颜色。
`[K1-T113]` (host.cpp:315) 清理。

---

## 进阶任务

- 把输出颜色从红色改为基于 `(x/width, y/height)` 的 UV 渐变
- 在 raygen 里调用 `optixTrace`，miss program 返回蓝色背景
- 添加 closest-hit program，打到一个三角形时返回绿色

---

## 验收点

- 编译无错误，`programs.optixir` 成功生成
- 运行无 OptiX API error（log callback 无 level<=2 输出）
- `output_k1.ppm` 打开后是纯红色 1024×1024 图像
- 用 Nsight Compute 可见 `optixLaunch` 触发的 kernel grid 配置

---

## 常见问题

**Q: nvcc 报 `--optix-ir: unrecognized option`**
A: CUDA Toolkit 版本过旧。需要 CUDA 11.7+（推荐 13.x）。
   执行 `nvcc --version` 确认版本。

**Q: `optixInit()` 返回 `OPTIX_ERROR_DRIVER_VERSION_INSUFFICIENT`**
A: 显卡驱动版本不够新。OptiX 8.1 需要驱动 >= 535.x（Windows）。

**Q: SBT record alignment 错误**
A: SBT record 必须 16 字节对齐。使用模板 `SbtRecord<T>` 并加
   `alignas(OPTIX_SBT_RECORD_ALIGNMENT)` 保证对齐。

**Q: `output_k1.ppm` 是黑色或全零**
A: 检查 `params.output` 指针是否正确上传到 device，以及 `cudaStreamSynchronize`
   是否在 `cudaMemcpy` 前调用。

---

## 参考资料

- OptiX 8 Programming Guide: https://raytracing-docs.nvidia.com/optix8/guide/index.html
- OptiX SDK samples: `<SDK_ROOT>/SDK/` 目录下 `optixHello` 示例
- CUDA Driver API 文档: https://docs.nvidia.com/cuda/cuda-driver-api/

---

## 输出对照（printf / std::puts 原文）

- `[K1-T53]` (host.cpp:21) 原文：`[K1] GPU_STUDY_NO_OPTIX=1: OptiX SDK 未安装。` → 现：`[K1] GPU_STUDY_NO_OPTIX=1: OptiX SDK is not installed.`
- `[K1-T54]` (host.cpp:23) 原文：`     请参考 README.md 的 'OptiX SDK 安装' 一节完成安装后重新配置 CMake。` → 现：`     See README.md section 'OptiX SDK installation' and re-configure CMake.`
- `[K1-T63]` (host.cpp:83) 原文：`无法打开文件: %s\n` → 现：`Cannot open file: %s\n`
- `[K1-T71]` (host.cpp:137) 原文：`  输出已写入: %s\n` → 现：`  Output written: %s\n`
- `[K1-T73]` (host.cpp:144) 原文：`[K1] hello_optix_pipeline` → 现：保持英文不变
- `[K1-T83]` (host.cpp:171) 原文：`  OptiX device context 创建成功` → 现：`  OptiX device context created`
- `[K1-T86]` (host.cpp:176) 原文：`  加载 .optixir: %s\n` → 现：`  Loading .optixir: %s\n`
- `[K1-T91]` (host.cpp:210) 原文：`  OptiX module 创建成功` → 现：`  OptiX module created`
- `[K1-T94]` (host.cpp:227) 原文：`  raygen program group 创建成功` → 现：`  raygen program group created`
- `[K1-T97]` (host.cpp:246) 原文：`  pipeline 创建成功` → 现：`  pipeline created`
- `[K1-T107]` (host.cpp:298) 原文：`  optixLaunch 完成` → 现：`  optixLaunch finished`
- `[K1-T114]` (host.cpp:330) 原文：`[K1] 完成。` → 现：`[K1] done.`
