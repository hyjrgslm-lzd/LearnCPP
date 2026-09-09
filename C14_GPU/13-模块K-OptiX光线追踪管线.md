# 13 模块 K：OptiX 光线追踪管线

## 模块目标

这个模块只做一件事：从零开始把 OptiX 8.x 完整的光线追踪管线建起来。

你会先装 OptiX SDK（手动安装，NV 许可协议禁止 FetchContent），再从 host 侧 boilerplate 开始（context 创建、module 加载、program group 注册），然后写 OptiX kernel 用 `nvcc --optix-ir` 编译成中间格式，最后用 Shader Binding Table（SBT）三段式组织 raygen/miss/hit 程序，完整感受光线追踪管线的"代码 + 数据 + 执行"三元组。

## 前置知识

- 完成模块 A–F，熟悉 CUDA kernel 编译和 Nsight 调试。
- 理解 GPU 的"批量数据驱动执行"模型；认可"kernel 是描述性的、launch 才是触发"。
- 知道什么是加速结构（BVH）、ray tracing 的基本概念（射出光线、求交、材质计算）。
- 能独立用 CMake 组织多文件 CUDA 项目。

## 模块完成标准

做完本模块，你至少要能说清楚：

1. OptiX 8.x 的五大 program type（raygen / intersection / any-hit / closest-hit / miss），各自职责边界。
2. Shader Binding Table（SBT）三段式（raygen / miss / hitgroup）是什么，为什么需要这种间接寻址。
3. 为什么 OptiX kernel 要用 `nvcc --optix-ir` 编译成 `.optixir`，而不是普通 PTX。
4. Acceleration Structure（GAS + IAS）如何表达场景几何，与 BVH 的关系。
5. `optixTrace` payload 机制如何让 raygen 和 hit program 通过隐式参数通讯。
6. OptiX Denoiser 的输入格式要求、output buffer layout、与 CUDA kernel 的互操作边界。

## 硬件与工具链要求

- **GPU**：Compute Capability ≥ sm_60；sm_75+（Turing）强烈推荐；sm_90a（Hopper）为最佳测试平台。
- **CUDA Toolkit**：13.x 或更新。
- **OptiX SDK**：8.0 或 8.1；**必须手动从 developer.nvidia.com 下载安装**（许可协议限制，无法通过 FetchContent 自动获取）。
  - 安装后设置环境变量 `OPTIX_SDK_ROOT` 指向 SDK 根目录。
  - 验证：`%OPTIX_SDK_ROOT%/include/optix.h` 存在。
- **CMake**：3.28+。
- **宿主编译器**：Visual Studio 2026（MSVC 19.5x）。
- **工具链**：Nsight Compute（用 `ncu --set full` 采集 ray-tracing 相关指标），`compute-sanitizer`。

---

## 练习 K1：hello_optix_pipeline

### 目标

写出第一个 OptiX 8 pipeline：最小的 host 侧 boilerplate 加载 SDK、创建 context、注册 module、组织 program group、构建 pipeline，再从 raygen 射出固定颜色光线到 output buffer。感受 OptiX 的"声明式管线"和"间接调用"的规模。

### 前置理解

- 你知道 CUDA kernel 编译的基本流程（`.cu` → PTX → SASS）。
- 你接受"OptiX kernel 是特殊的 PTX 中间形式"，需要额外编译步骤。
- 你能理解"program group 是一组相关函数指针的统一入口"。

### 必做任务

1. **SDK 安装与环境变量**：确保 `OPTIX_SDK_ROOT` 已设置，CMake `find_path(OPTIX_INCLUDE_DIR optix.h HINTS $ENV{OPTIX_SDK_ROOT}/include)` 能找到头文件。
   ```
   // TODO [必做] 验证 ${OPTIX_SDK_ROOT}/include/optix.h 可访问
   ```

2. **OptiX 初始化**：在 `main.cu` 的 host 代码中调用 `optixInit()`，创建 CUDA context 后再创建 OptiX device context。
   ```cpp
   // TODO [必做] optixInit()
   // TODO [必做] optixDeviceContextCreate(cuCtx, &options, &optix_ctx)
   ```

3. **编译 raygen kernel**：在 `raygen.cu` 中写一个最小的 raygen program：接收 output buffer，写一个固定 RGBA 颜色（例如红色 (1,0,0,1)）。
   ```cuda
   // TODO [必做] extern "C" __global__ void __raygen__hello()
   // 内部写 output[idx] = make_float4(1,0,0,1)
   ```

4. **OptiX IR 编译**：在 `CMakeLists.txt` 中添加 custom command 用 `nvcc --optix-ir` 编译 `raygen.cu` → `raygen.optixir`。
   ```cmake
   # TODO [必做] 添加 custom_command: nvcc --optix-ir raygen.cu -o raygen.optixir
   ```

5. **Module 加载**：host 代码从 `.optixir` 文件读取二进制数据，调用 `optixModuleCreate()` 加载到 OptiX context。
   ```cpp
   // TODO [必做] 读取 raygen.optixir 文件内容
   // TODO [必做] optixModuleCreate(optix_ctx, &module_opts, code, code_size, ..., &module)
   ```

6. **Program Group 创建**：注册 raygen program group（`OptixProgramGroupDesc.raygen.module` 指向 module，`.raygen.entryFunctionName` = `"__raygen__hello"`）。
   ```cpp
   // TODO [必做] 构造 OptixProgramGroupDesc，type = OPTIX_PROGRAM_GROUP_KIND_RAYGEN
   // TODO [必做] optixProgramGroupCreate(optix_ctx, &pg_desc, 1, &pg_options, ..., &pg)
   ```

7. **Pipeline 构建**：用 program group 创建 pipeline（`optixPipelineCreate`），设置 rayPayload 大小（例如 0，因为 hello 不用 payload）。
   ```cpp
   // TODO [必做] OptixPipeline pipeline = optixPipelineCreate(...)
   // 设置 pipeline_compile_options.payloadValues = 0
   ```

8. **SBT 构造**（简化版）：分配一个 raygen SBT record，写入 program group handle，上传到 GPU。
   ```cpp
   // TODO [必做] 分配 host 端 raygen SBT record buffer
   // TODO [必做] 填入 OptixSbtRecord<RayGenData> 
   // TODO [必做] cudaMemcpy 上传到 device
   ```

9. **光线追踪启动**：从 host 调用 `optixLaunch(pipeline, stream, sbt, ...)` 生成 1024×1024 的 output。
   ```cpp
   // TODO [必做] optixLaunch(pipeline, stream, d_sbt, sbt_offset, sbt_stride, ...)
   // 参数：raygen_record_offset=0, raygen_record_stride=sizeof(RayGenRecord)
   ```

10. **结果保存**：将 output buffer 读回 host，写成 PPM 或 PNG（可用 stb_image_write.h）。
    ```cpp
    // TODO [必做] cudaMemcpy(h_output, d_output, size, cudaMemcpyDeviceToHost)
    // TODO [必做] 写入图像文件
    ```

### 进阶任务

- 把 output 颜色从红色改为从 `threadIdx.x / blockDim.x` 生成的渐变。
- 在 raygen 里调用 `optixTrace` 与 miss program 通讯，miss 返回背景颜色。
- 添加一个 closest-hit program，打到面片时返回不同颜色。

### 验收点

- 编译无错误（`.optixir` 产生，host 代码链接成功）。
- 程序运行无 OptiX API error（通过 log callback 验证）。
- 输出 PPM/PNG 是纯红色（1024×1024 分辨率）。
- 你能用 Nsight Compute 看到 `optixLaunch` 调用的 kernel grid 配置和 occupancy。

### 观察点

- OptiX host 侧 boilerplate 远大于一个普通 CUDA kernel launch（context、module、program group、SBT 一系列步骤）。
- `.optixir` 编译输出是 OptiX 专有的中间格式，不是标准 PTX。
- SBT 本质是一个"函数指针 + 数据"的数组，OptiX 用它来间接调用对应的 program。
- raygen program 在 OptiX 模型里是"起点"，由 `optixLaunch` 直接启动，而不是从 host 明确指定 kernel 名。

### 常见坑

1. **`OPTIX_SDK_ROOT` 环境变量未设**：CMake 找不到 `optix.h`，link 失败。解决：在 PowerShell/cmd 里 `$env:OPTIX_SDK_ROOT = "C:\...SDK"` 或写入系统环境变量。
2. **nvcc `--optix-ir` 编译失败**：参数遗漏或路径错误。需确保 `nvcc --version` 支持 OptiX IR（通常 CUDA 12.x+ 支持）；如果不支持，改用 `-rdc=true --compile-ptx` 然后用 optixirBuild（较复杂）。
3. **`.optixir` 文件找不到**：CMake custom command 没有在 build 前执行，或输出路径错误。确保 `${CMAKE_CURRENT_BINARY_DIR}/raygen.optixir` 作为 dependency 被主 executable 依赖。
4. **SBT record stride 计算错误**：OptiX 要求 record offset 和 stride 必须对齐（通常 16 字节倍数）。如果不对齐，`optixLaunch` 会返回 `OPTIX_ERROR_SBT_RECORD_ALIGNMENT`。
5. **GAS build 后忘记 compact**：初次 build 的 acceleration structure 可能很大；如果要多次 launch，应该调用 `optixAccelCompact` 节省显存（虽然这题还没用 GAS，但容易在 K2 犯这个错）。
6. **`optixTrace` payload 槽位数不匹配**：raygen 声明 `__shared__` payload 4 个 uint，但 pipeline 设的 `payloadValues` 是 2，会导致 undefined behavior。务必在 pipeline config 和 raygen/closest-hit 里保持一致。
7. **device context log callback 未设**：OptiX 错误会被静默忽略，导致难以调试。建议在 `optixDeviceContextCreate` 时传入 `callbackFunction`，捕获 log 消息。
8. **OptiX 与 CUDA stream 混用同步问题**：`optixLaunch` 在指定 stream 上异步执行；如果后续 `cudaMemcpy` 不带 `cudaMemcpyDeviceToHost` 的 stream 同步，可能读到过时数据。务必显式 `cudaStreamSynchronize(stream)` 或使用 event。
9. **Windows OptiX 7.x vs 8.x 头文件路径差异**：SDK 目录结构不同（7.x 是 `SDK/include/optix_7_device.h`，8.x 是 `SDK/include/optix.h` + `optix_device.h` 分离）。确保 `#include <optix.h>` 且环境变量指向正确版本。
10. **Denoiser 输入 buffer layout 必须是 float4 RGBA**：如果 raygen kernel 输出是 uint8 RGBA，denoiser 会返回错误。务必检查输出格式，或在 raygen 内部转换为 float4。
11. **OptiX kernel 里调用 `printf` 输出被截断**：device-side printf buffer 大小有限制（通常 1 MB）。不要在 raygen 里对每个像素都 printf；改用 log callback 或后处理。
12. **忘记 `-Xcompiler "/permissive-"` 或类似 MSVC 兼容性标志**：OptiX kernel 用 C++17/20 特性（如 `__nv_fp8_e4m3` 类型），MSVC 需要宽松模式。

### 提示

- OptiX initialization 最小代码框架：
  ```cpp
  CUresult res = cuInit(0);
  CUdevice cuDevice = 0;
  cuDeviceGet(&cuDevice, 0);
  CUcontext cuContext = nullptr;
  cuCtxCreate(&cuContext, 0, cuDevice);
  
  OptixDeviceContext optix_ctx = nullptr;
  OptixDeviceContextOptions options = {};
  options.logCallbackFunction = [](unsigned int level, const char* tag, const char* message, void* cbdata) {
    std::fprintf(stderr, "[%u] %s: %s\n", level, tag, message);
  };
  optixDeviceContextCreate(cuContext, &options, &optix_ctx);
  ```

- raygen kernel 最小代码：
  ```cuda
  extern "C" __global__ void __raygen__hello() {
    unsigned int x = optixGetLaunchIndex().x;
    unsigned int y = optixGetLaunchIndex().y;
    float4* output = (float4*)optixGetAttribute_0();
    unsigned int w = optixGetLaunchDimensions().x;
    output[y * w + x] = make_float4(1.0f, 0.0f, 0.0f, 1.0f);
  }
  ```

- CMake 编译 `.optixir` 的 custom command 示例：
  ```cmake
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/raygen.optixir
    COMMAND ${CMAKE_CUDA_COMPILER} --optix-ir 
            ${CMAKE_CURRENT_SOURCE_DIR}/raygen.cu
            -o ${CMAKE_CURRENT_BINARY_DIR}/raygen.optixir
            -I${OPTIX_INCLUDE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/raygen.cu
  )
  ```

### 复盘问题

1. OptiX device context 和 CUDA context 的关系是什么？为什么要在 `cuCtxCreate` 后再 `optixDeviceContextCreate`？
2. 为什么 raygen program 需要从 SBT 间接调用，而不是直接在 host 侧指定函数名或指针？
3. `.optixir` 中间格式相比普通 PTX 的优势是什么？为什么 NV 为 OptiX 专门设计这一层？
4. 如果你在 pipeline 创建时指定 `payloadValues = 2`，但 raygen 里用了 4 个 payload 槽位，会发生什么？
5. SBT record 的 offset 和 stride 分别控制什么？为什么对齐要求这么严格？
6. 如果要支持多个 raygen program（例如"主 raygen"和"debug raygen"），SBT 应该如何扩展？

### 对应官方参考

- OptiX 8 Programming Guide：https://raytracing-docs.nvidia.com/optix8/guide/index.html
- OptiX SDK 源码示例：`OptiX SDK 8.x/samples/` 下 `hello.cu` / `sphere_tracing.cu`
- OptiX Toolkit（社区维护样例）：https://github.com/NVIDIA/optix-toolkit
- CUDA Programming Guide - OptiX 章节：https://docs.nvidia.com/cuda/cuda-c-programming-guide/

---

## 练习 K2：sbt_and_acceleration_structure

### 目标

把 K1 扩展成一个真实场景：用 GAS（Geometry Acceleration Structure）表达三角形网格，用 IAS（Instance Acceleration Structure）支持多个实例，用三段式 SBT 组织 raygen/miss/hitgroup 程序，在光线打到面片时 hit program 计算法线并输出法线可视化。

### 前置理解

- 完成 K1，熟悉 OptiX pipeline 基本框架。
- 理解 BVH（二叉搜索树）和 acceleration structure 的概念。
- 知道"法线"在渲染中的用途（光照计算、表面属性表达）。

### 必做任务

1. **定义场景几何**：创建一个 Cornell Box（立方体房间，6 个面），用三角形网格表达。顶点数据用 `float3` 数组，索引用 `uint3` 数组。
   ```cpp
   // TODO [必做] 定义 cornell_vertices[] 和 cornell_indices[]
   // Cornell Box：白墙×4、红墙×1、蓝墙×1
   ```

2. **GAS 构建**：用 `optixAccelBuild` 创建 Geometry Acceleration Structure，加载三角形网格。配置 `OptixAccelBuildOptions`、`OptixBuildInput` 指向顶点/索引缓冲。
   ```cpp
   // TODO [必做] 分配 d_vertices, d_indices 到 device
   // TODO [必做] 构造 OptixBuildInput（三角形 mesh）
   // TODO [必做] optixAccelBuild(..., &build_input, ..., &gas_handle)
   ```

3. **IAS 构建**（可选但推荐）：如果只有一个 GAS，可以跳过；如果要支持多个实例或变换，创建 IAS。
   ```cpp
   // TODO [必做] 如果用多个 instance，构造 OptixInstance 数组
   // TODO [必做] optixAccelBuild(..., &instance_build_input, ..., &ias_handle)
   ```

4. **Miss Program 编写**：写一个 miss program，光线没有打到任何几何体时执行，返回背景颜色（例如天蓝色）。
   ```cuda
   // TODO [必做] extern "C" __global__ void __miss__background()
   // 根据光线方向返回渐变背景
   ```

5. **Closest-Hit Program 编写**：写一个 closest-hit program，光线打到面片时执行。计算三角形法线，通过 payload 传回 raygen，raygen 将法线可视化为 RGB 颜色。
   ```cuda
   // TODO [必做] extern "C" __global__ void __closesthit__radiance()
   // 计算三角形法线、写入 payload
   ```

6. **编译多个 OptiX kernel**：在 `CMakeLists.txt` 中编译 `raygen.cu`、`miss.cu`、`closesthit.cu` 成对应 `.optixir` 文件。
   ```cmake
   # TODO [必做] 三个 custom_command 分别编译三个 kernel
   ```

7. **Program Group 注册**：创建三种 program group（raygen / miss / hitgroup）。hitgroup 包含 `closest-hit` 和可选的 `intersection`（对三角形网格，intersection 由 OptiX 内置，不需要自定义）。
   ```cpp
   // TODO [必做] 创建 raygen program group
   // TODO [必做] 创建 miss program group（指向 __miss__background）
   // TODO [必做] 创建 hitgroup program group（指向 __closesthit__radiance）
   ```

8. **SBT 三段式构造**：
   - **Raygen 段**：1 条 record（raygen program）。
   - **Miss 段**：1 条 record（miss program）。
   - **Hitgroup 段**：N 条 records（每个可打到的材质/对象一条）。
   
   ```cpp
   // TODO [必做] 分配 raygen SBT buffer（大小 = sizeof(RayGenRecord)）
   // TODO [必做] 分配 miss SBT buffer（大小 = sizeof(MissRecord)）
   // TODO [必做] 分配 hitgroup SBT buffer（大小 = N * sizeof(HitGroupRecord)）
   // TODO [必做] 组织成 OptixShaderBindingTable，设置 offset 和 stride
   ```

9. **Pipeline 更新**：新 pipeline 需要包含 raygen + miss + hitgroup program groups。
   ```cpp
   // TODO [必做] 重新 optixPipelineCreate，加入三个 program group
   // 设置 payloadValues 足够大，例如 2-4 个 uint
   ```

10. **Raygen 改进**：raygen 现在要生成光线方向、调用 `optixTrace`、处理返回值。
    ```cuda
    // TODO [必做] 在 raygen 中调用 optixTrace，传入 GAS handle 和光线起点/方向
    // 设置 payload 接收 miss/closest-hit 的结果
    ```

11. **结果可视化**：raygen 收到 miss/closest-hit 的颜色信息，写入 output buffer。
    ```cpp
    // TODO [必做] 将光线追踪结果（法线或背景）写入 output
    ```

### 进阶任务

- 添加第二个 closest-hit program，计算不同的材质（例如镜面反射 vs Lambertian）。
- 支持多个 instance，每个 instance 有不同的变换矩阵和材质 ID。
- 在 closest-hit 中递归调用 `optixTrace` 计算阴影光线或反射光线。
- 使用 `optixAccelCompact` 压缩 GAS 以节省显存。

### 验收点

- 编译无错误，三个 `.optixir` 文件成功生成。
- 程序运行无 OptiX API error，output buffer 被正确填充。
- 输出图像显示 Cornell Box 的法线可视化：各面颜色应能区分（例如顶面白色→(0.5,0.5,1)，侧面红→(1,0,0) 偏移后）。
- 用 Nsight Compute 验证 hit program 被调用的频率和 occupancy。

### 观察点

- SBT 是 OptiX 管线中"数据 + 代码指针"的统一索引，使得不同几何体可以引用不同的 shader。
- 法线计算从三角形顶点通过 cross product 得到，体现了"最小化 kernel 复杂度"的设计（OptiX 负责找到打到的三角形，kernel 只需计算法线）。
- miss program 和 closest-hit program 都是通过 payload 与 raygen 通讯，而非返回值或全局变量。

### 常见坑

1. **GAS build input 顶点格式错误**：OptiX 要求 `OptixBuildInputTriangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3`，且顶点必须是 `float3` 数组（不是 `glm::vec3` 或其他）。
2. **索引数据类型不匹配**：`indexFormat` 要声明正确（`OPTIX_INDICES_FORMAT_UINT3`），索引数据必须是 `uint3` 数组。
3. **SBT record alignment 错误**：record 大小必须是 16 的倍数；如果 `sizeof(HitGroupRecord) % 16 != 0`，需要 padding。
4. **Program group 指针被覆盖**：创建完 program group 后，记住指针；后续 SBT 初始化需要这些指针。
5. **Pipeline 的 program group 数量和 SBT 段数不对应**：raygen 1 条、miss 1 条、hitgroup M 条，但 pipeline 里注册的顺序要和 SBT 对应。
6. **`optixTrace` 的 InstanceId 或 SBT offset 设错**：导致调用到错误的 hit program。
7. **Closest-hit 中法线计算用了错误的顶点索引**：OptiX 在 `optixGetAttribute_0/1/2` 中提供原始三角形顶点属性，但如果访问方式不对会读到垃圾。
8. **没有在 pipeline 或 module 创建时启用 debug 符号**：导致 Nsight 看不到源码行号。建议 `--generate-line-info` 或 `-g`。
9. **Payload 初始化在 raygen 中忘记**：`optixTrace` 前必须初始化 payload（至少清 0），否则 miss/hit program 读到未定义值。
10. **GAS build 时分配的 temp buffer 太小**：OptiX 会返回需要多少 temp 空间的提示，第二次 build 前要分配足够的 buffer。
11. **Instance transform 矩阵格式错误**：OptiX 用行优先 float3x4 矩阵，不是列优先；误混淆会导致变换应用不正确。

### 提示

- Cornell Box 顶点数据框架：
  ```cpp
  float3 cornell_vertices[] = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},  // back
    {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1},      // front
    // ... 标记顶点，共 8 个或更多（如果多面拆分）
  };
  uint3 cornell_indices[] = {
    {0, 1, 2}, {0, 2, 3}, // back face, 两个三角形
    // ...
  };
  ```

- Closest-hit 法线计算：
  ```cuda
  extern "C" __global__ void __closesthit__radiance() {
    const float3* v0 = (const float3*)optixGetAttribute_0();
    const float3* v1 = (const float3*)optixGetAttribute_1();
    const float3* v2 = (const float3*)optixGetAttribute_2();
    float3 edge1 = *v1 - *v0;
    float3 edge2 = *v2 - *v0;
    float3 normal = normalize(cross(edge1, edge2));
    // 写入 payload：payload[0] = __float_as_uint(normal.x) 等
  }
  ```

### 复盘问题

1. SBT 的三段式（raygen / miss / hitgroup）分别对应什么执行流程中的哪些环节？
2. 如果有 M 个不同材质，SBT hitgroup 段应该有多少条 record？
3. GAS 和 IAS 的区别是什么？什么时候必须用 IAS？
4. OptiX 在查找要执行的 closest-hit program 时，是通过什么机制从几何体 ID 映射到 SBT record？
5. 为什么 miss program 和 closest-hit program 都需要存在，但 raygen 只需要一个？
6. 如果想让不同面有不同的法线可视化颜色（例如红面输出红，白面输出白），应该如何组织 SBT？

### 对应官方参考

- OptiX Programming Guide - Acceleration Structures：https://raytracing-docs.nvidia.com/optix8/guide/index.html#acceleration_structures
- OptiX Programming Guide - Shader Binding Table：https://raytracing-docs.nvidia.com/optix8/guide/index.html#shader_binding_table
- OptiX SDK - sphere_tracing 示例
- NVIDIA Blog - Getting Started with OptiX 8：developer.nvidia.com/blog

---

## 练习 K3：intersection_anyhit_closesthit_miss

### 目标

完整展示 OptiX 的五大 program type 的分工：raygen（起点）、intersection（自定义求交）、any-hit（alpha 测试、透明度处理）、closest-hit（最终着色）、miss（背景）。实现一个简单的 Lambertian 材质 + 点光源的直接照明，体会"可配置的着色管线"。

### 前置理解

- 完成 K1 和 K2，熟悉基本 pipeline、GAS、SBT、raygen/miss/closest-hit。
- 理解 any-hit 的作用：光线遇到任何潜在的几何体首先调用 any-hit（不一定是最近的），可以选择继续追踪或终止。
- 了解 intersection program 如何自定义求交逻辑（例如射线与隐式曲面、体积、粒子的求交）。

### 必做任务

1. **扩展场景**：在 K2 的 Cornell Box 基础上，添加一个点光源位置和强度。
   ```cpp
   // TODO [必做] 定义 struct PointLight { float3 pos; float3 intensity; }
   // TODO [必做] 在常量内存或通过 payload 传递光源信息到 hit program
   ```

2. **Any-Hit Program**：实现一个简单的 any-hit，用于透明度测试（例如如果物体有 alpha < 0.5，忽略交点继续追踪）。这题中可以全部通过（alpha = 1.0），但骨架必须存在。
   ```cuda
   // TODO [必做] extern "C" __global__ void __anyhit__opaque()
   // 检查 alpha 或贴图，决定是否 optixTerminateRay() / optixIgnoreIntersection()
   ```

3. **自定义 Intersection Program**（可选进阶）：添加一个自定义 primitive（例如球或光源本身），用 intersection program 计算射线与球的交点。
   ```cuda
   // TODO [进阶] extern "C" __global__ void __intersection__sphere()
   // 计算射线与球的交点 t，调用 optixReportIntersection(t, hit_kind)
   ```

4. **Closest-Hit 着色**：实现完整的 Lambertian 直接照明。
   - 计算三角形法线（从 K2）。
   - 计算点光源方向。
   - 计算 cosine term：`max(0, dot(normal, light_dir))`。
   - 计算阴影光线：从 hit point 向光源再次 `optixTrace`，检查是否被遮挡。
   - 如果未遮挡，返回 `Lambertian color * light intensity * cosine`。
   
   ```cuda
   // TODO [必做] 在 closest-hit 中计算 cosine term
   // TODO [必做] 在 closest-hit 中生成阴影光线（recursion depth = 1）
   // TODO [必做] miss/any-hit 时设置 shadow flag，closest-hit 检查是否被遮挡
   ```

5. **Payload 扩展**：payload 从简单的 float4 rgb 扩展为包含：RGB 颜色、shadow flag、recursion depth。
   ```cpp
   // TODO [必做] 定义 struct RayPayload { float3 rgb; uint shadow; uint depth; }
   // TODO [必做] pipeline 配置 payloadValues 足够大
   ```

6. **Miss Program 改进**：miss program 现在要检查 recursion depth，避免无限递归。如果深度 > 1（来自阴影光线），返回 0（黑色，表示被遮挡）；否则返回背景颜色。
   ```cuda
   // TODO [必做] __miss__background 检查 payload.depth，决定返回黑还是蓝
   ```

7. **Raygen 改进**：raygen 现在要生成初级光线（depth=0），初始化 payload，调用 `optixTrace`。
   ```cuda
   // TODO [必做] raygen 设置 payload.depth = 0，调用 optixTrace
   ```

8. **编译与链接**：编译 raygen.cu、any_hit.cu、closest_hit.cu、miss.cu 成 `.optixir`，注册所有 program group。
   ```cmake
   # TODO [必做] 四个 kernel 对应四个 custom_command
   ```

9. **SBT 组织**：raygen 1 条、miss 1 条、hitgroup 有 N 条（每个几何体一条，包含 any-hit + closest-hit）。
   ```cpp
   // TODO [必做] 重新组织 SBT，hitgroup record 包含 any-hit + closest-hit 指针
   ```

10. **结果输出**：output buffer 应该显示 Cornell Box 的直接照明效果，可见阴影。

### 进阶任务

- 实现自定义 intersection（球光源），使光源本身可被光线打到。
- 改为 Phong 着色模型，加上镜面反射项。
- 支持多个点光源，遍历循环计算贡献。
- 实现 glossy 反射，调整 roughness 参数。

### 验收点

- 编译无错误，4 个 `.optixir` 文件成功生成。
- 程序运行无 OptiX API error。
- 输出图像显示 Cornell Box 的直接照明效果：光照不均（光源附近亮，远处暗），可见阴影。
- 用 Nsight Compute 验证：raygen、any-hit、closest-hit、miss 都被调用；recursion depth 不超过 2。

### 观察点

- Any-hit 提供了"提前终止"的机制（例如透明/遮挡检测），而 closest-hit 只在最终确定最近交点时调用。
- Intersection program 虽然这题未实现，但留下了骨架，为"自定义几何体"开放了接口。
- 用 payload 传递 shadow flag 和 recursion depth，避免了全局状态污染。

### 常见坑

1. **Payload 数据类型转换**：payload 是 uint 数组，float 数据要用 `__float_as_uint` 和 `__uint_as_float` 转换，易出错。
2. **递归深度无限增长**：如果忘记在 closest-hit 中增加 payload.depth，阴影光线会再生成阴影光线，无限循环。
3. **Any-hit 中调用 `optixIgnoreIntersection()` 后仍设置 payload**：`optixIgnoreIntersection()` 会跳过当前交点，但如果代码仍尝试设置 payload 会被浪费（下次 closest-hit 时 payload 不是这次的值）。
4. **Light direction 未 normalize**：导致 dot product 计算错误，漫反射项不正确。
5. **Shadow ray 起点设在表面上**：如果不加偏移，shadow ray 会立即与该表面求交（数值误差），导致错误的阴影。需要 `hit_point + epsilon * normal`。
6. **Miss program 中读 payload 但初始化不正确**：如果初级光线 miss，payload 应该已被初始化，否则读到垃圾。
7. **Program group 的 any-hit 指针为 null 时行为未定义**：即使没有 any-hit，也要显式设为 nullptr（或在 SBT record 中标记"无 any-hit"）。
8. **Light intensity 单位混乱**：如果用太大的 intensity，输出会 saturate 成纯白；太小则不可见。需要合理缩放，或在 raygen 中做 tone mapping。
9. **Closest-hit 中阴影光线的方向反了**：应该从 hit point 指向 light 位置，不是反过来。
10. **Pipeline 的 payload 类型声明与代码不匹配**：pipeline 说 payloadValues=2 (8 bytes)，但代码用 float3 + uint (16 bytes)，会导致 payload overflow。

### 提示

- Payload 结构体与 uint 数组的映射：
  ```cpp
  struct RayPayload {
    float3 rgb;      // 占 12 bytes（3×4）
    uint depth;      // 占 4 bytes
  };
  // 总计 16 bytes = 4 uints，所以 payloadValues = 4
  
  // 在 kernel 中访问：
  extern "C" __global__ void __raygen__principal() {
    uint payload[4] = {0};
    // 或使用指针强转
    RayPayload* p = (RayPayload*)payload;
  }
  ```

- Lambertian 直接照明框架：
  ```cuda
  extern "C" __global__ void __closesthit__radiance() {
    // 获取法线
    float3 normal = /* compute from vertices */;
    
    // 光源信息（通过 grid constant 或常量内存）
    float3 light_pos = /* ... */;
    float3 light_intensity = /* ... */;
    
    // 计算光源方向
    float3 light_dir = normalize(light_pos - hit_point);
    float cosine = max(0.0f, dot(normal, light_dir));
    
    // 生成阴影光线
    uint shadow_payload[4] = {0};
    shadow_payload[3] = 1; // depth = 1（表示阴影光线）
    optixTrace(params.handle_gas, hit_point + normal * 1e-4f, light_dir,
               0.0f, 1e6f, 0.0f, OptixVisibilityMask(255), 
               OPTIX_RAY_FLAG_DISABLE_ANYHIT, 0, 1, 0, shadow_payload);
    
    // 如果 shadow_payload[3] == 0，表示 hit 了东西（被遮挡）
    float visibility = (shadow_payload[3] > 0) ? 1.0f : 0.0f;
    
    // 设置输出颜色
    float3 color = Lambertian_color * light_intensity * cosine * visibility;
  }
  ```

### 复盘问题

1. Any-hit 和 closest-hit 各自适合处理什么类型的逻辑？为什么要分开？
2. Payload 在递归调用 `optixTrace` 时是否被重新初始化？如何在 miss/any-hit 中区分是否来自阴影光线？
3. 为什么需要对 shadow ray 起点加法线偏移？如果不加会发生什么？
4. Intersection program 的典型应用场景是什么？
5. 如果要实现透明物体（alpha < 1），any-hit 应该如何修改？
6. 这题中，raygen、closest-hit、miss、any-hit 各被调用了多少次（相对于像素数量）？

### 对应官方参考

- OptiX Programming Guide - Program Types：https://raytracing-docs.nvidia.com/optix8/guide/index.html#program_types
- OptiX Programming Guide - Recursion and Depth：https://raytracing-docs.nvidia.com/optix8/guide/index.html#recursion
- NVIDIA Blog - Building Efficient Ray Tracing Pipelines with OptiX 8

---

## 练习 K4：optix_denoiser_and_cuda_interop

### 目标

集成 OptiX Denoiser，从 ray tracing 的高噪声结果（例如蒙特卡罗路径追踪 32 spp）一步到位去噪到低噪声图像。同时展示 OptiX 与 CUDA kernel 的互操作：OptiX output → CUDA tonemap kernel → HDR 文件输出。

### 前置理解

- 完成 K1–K3，熟悉完整 OptiX pipeline。
- 理解"噪声"来自蒙特卡罗采样的方差；去噪是通过 AI 模型学习空间相关性来滤波。
- 知道什么是 tonemap（将 HDR 色彩空间映射到 LDR 显示范围）。

### 必做任务

1. **生成高噪声图像**：修改 K3 的 raygen，改为蒙特卡罗路径追踪（每像素随机 32 条光线，取平均）。因为没有重要性采样，噪声会很明显。
   ```cuda
   // TODO [必做] raygen 循环 32 次，每次生成随机方向光线，累积颜色平均
   ```

2. **Denoiser 初始化**：在 host 代码中调用 `optixDenoiserCreate` 创建 denoiser。
   ```cpp
   // TODO [必做] OptixDenoiser denoiser = optixDenoiserCreate(optix_ctx, ...);
   ```

3. **Denoiser 参数配置**：设置 `OptixDenoiserOptions`（例如 `inputKind = OPTIX_DENOISER_INPUT_RGB`），设置 `OptixDenoiserParams` 的 blend（1.0 表示完全相信 denoiser，0.0 表示保留原图）。
   ```cpp
   // TODO [必做] OptixDenoiserParams denoiser_params = {};
   // denoiser_params.blendFactor = 1.0f; // 全部相信 denoiser
   ```

4. **Albedo 和 Normal Buffer**（可选但推荐）：Denoiser 的效果可以用 albedo（表面固有颜色）和 normal 进一步改善。在 raygen/closest-hit 中输出这两个 buffer。
   ```cuda
   // TODO [必做] 在 raygen 中初始化 albedo buffer（例如全 1）
   // TODO [必做] 在 closest-hit 中输出 normal 到 normal buffer
   ```

5. **GPU 内存分配**：分配 device 端的 color / albedo / normal buffer（float4 格式）。
   ```cpp
   // TODO [必做] cudaMalloc 三个 buffer，尺寸 width * height * sizeof(float4)
   ```

6. **Denoiser Setup**：调用 `optixDenoiserSetup` 分配 denoiser 内部的工作空间。
   ```cpp
   // TODO [必做] optixDenoiserSetup(denoiser, stream, width, height, &denoiser_state, ...);
   // 记录返回的 state_size 和 scratch_size，分配对应 buffer
   ```

7. **Denoiser Invoke**：调用 `optixDenoiserInvoke` 对 color buffer 进行去噪，输出到新的 buffer。
   ```cpp
   // TODO [必做] optixDenoiserInvoke(denoiser, stream, denoiser_params, 
   //                    denoiser_state, state_size, 
   //                    input_layers, num_input_layers, 
   //                    output_offset, output_layers, num_output_layers, 
   //                    scratch, scratch_size);
   ```

8. **Tonemap Kernel**：编写一个 CUDA kernel 将 denoised 的 HDR 图像（float4）转换为 LDR（uint8 RGB）。使用简单的 ACES filmic tonemap 或 gamma correction。
   ```cuda
   // TODO [必做] __global__ void kernel_tonemap(float4* in, uint8_t* out, int width, int height)
   // 实现 ACES tonemap 或 gamma correction
   ```

9. **Interop 调用**：从 host 调用 tonemap kernel，将 denoised 结果转换为 uint8。
   ```cpp
   // TODO [必做] kernel_tonemap<<<grid, block>>>(d_denoised, d_ldr, width, height);
   // TODO [必做] 读回 d_ldr 到 host
   ```

10. **结果输出**：将 LDR 图像写成 PNG 或 HDR。
    ```cpp
    // TODO [必做] 用 stb_image_write 或其他库保存 PNG/HDR 文件
    ```

### 进阶任务

- 实现自适应采样：高噪声区域多采样，低噪声区域少采样。
- 支持 albedo-guided 和 normal-guided denoising（传入 albedo/normal buffer 给 denoiser）。
- 比较 denoiser blend factor 的效果（0.0, 0.5, 1.0）。
- 实现多帧累积 + 运动补偿，进一步降低噪声。

### 验收点

- 编译无错误。
- 程序运行无 OptiX API error 或 CUDA error。
- 输出有两个 PNG/HDR：一个是原始 32 spp 高噪声（参考），一个是 denoised 低噪声。
- 用肉眼对比，denoised 版本明显更平滑，细节仍保留。
- Tonemap 正确：denoised float4 输出应该映射到 [0, 255] 的 uint8 范围，无 overflow/underflow。

### 观察点

- Denoiser 是 OptiX 内置的 AI 去噪模块，无需自己训练模型。
- Albedo 和 normal 作为"指导"信息，帮助 denoiser 理解表面属性，从而更好地保留细节。
- Tonemap 的目的是将 HDR 亮度范围压缩到显示器能显示的范围，同时保留视觉感知的相对对比度。

### 常见坑

1. **Denoiser input buffer 格式必须是 float4 RGBA**：如果输出是 uint8 或 float3，denoiser 会返回错误。
2. **Denoiser state 和 scratch buffer 大小计算错误**：如果分配过小，invoke 时会访问越界。务必用 `optixDenoiserSetup` 返回的 size 值。
3. **Albedo/normal buffer 未初始化**：如果传入 null 指针给 denoiser，效果会差。即使不用 guided denoising，也要分配并初始化为合理值（例如 normal = (0.5, 0.5, 1)）。
4. **Blend factor 理解错误**：`blendFactor = 1.0` 表示完全用 denoiser 结果，`= 0.0` 表示保留原图。不是"混合比例"。
5. **没有在 tonemap 前 clamp 输入**：如果 denoised float4 的某些值是 NaN 或无穷大，tonemap 会产生垃圾输出。
6. **Tonemap kernel 的 gamma 值选错**：常见的是 gamma = 2.2（逆伽马），如果用 2.0 或其他，视觉效果会偏暗或偏亮。
7. **ACES tonemap 公式写错**：ACES 的标准公式是 `(x * (a*x + b)) / (x * (c*x + d) + e)`，误写很容易导致渐变失真。
8. **Denoiser 调用时没有 stream synchronize**：denoiser 异步执行，如果立即读 output buffer，可能读到过时数据。需要 `cudaStreamSynchronize(stream)` 或 event。
9. **输出图像尺寸与 denoiser setup 的尺寸不匹配**：setup 时声称是 1024×1024，但实际输出 512×512，invoke 时会出错。
10. **PNG 编码时 byte order 错误**：`stb_image_write` 期望 RGBA 或 RGB 格式，如果传入 BGRA 或其他顺序，颜色会反转。
11. **没有处理 denoiser 不支持的图像分辨率**：某些 denoiser 版本对分辨率有限制（例如必须是 16 的倍数）。

### 提示

- Denoiser 初始化框架：
  ```cpp
  OptixDenoiser denoiser = nullptr;
  OptixDenoiserOptions denoiser_options = {};
  denoiser_options.inputKind = OPTIX_DENOISER_INPUT_RGB;
  denoiser_options.pixelFormat = OPTIX_PIXEL_FORMAT_FLOAT4;
  OPTIX_CHECK(optixDenoiserCreate(optix_ctx, &denoiser_options, &denoiser));
  
  OptixDenoiserSizes denoiser_sizes;
  OPTIX_CHECK(optixDenoiserComputeMemoryResources(denoiser, width, height, &denoiser_sizes));
  
  CUdeviceptr d_denoiser_state = 0;
  CUDA_CHECK(cudaMalloc(&d_denoiser_state, denoiser_sizes.stateSizeInBytes));
  
  CUdeviceptr d_scratch = 0;
  CUDA_CHECK(cudaMalloc(&d_scratch, denoiser_sizes.scratchSizeInBytes));
  
  OPTIX_CHECK(optixDenoiserSetup(denoiser, stream, width, height, 
                                  d_denoiser_state, denoiser_sizes.stateSizeInBytes,
                                  d_scratch, denoiser_sizes.scratchSizeInBytes));
  ```

- ACES tonemap kernel：
  ```cuda
  __global__ void kernel_tonemap_aces(float4* in, uint8_t* out, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= width || y >= height) return;
    
    float4 hdr = in[y * width + x];
    float3 color = {hdr.x, hdr.y, hdr.z};
    
    // ACES filmic tonemap
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    
    color = (color * (a * color + b)) / (color * (c * color + d) + e);
    color = pow(color, 1.0f / 2.2f); // gamma correction
    
    uint8_t r = (uint8_t)(__saturatef(color.x) * 255);
    uint8_t g = (uint8_t)(__saturatef(color.y) * 255);
    uint8_t b = (uint8_t)(__saturatef(color.z) * 255);
    
    out[y * width + x * 3 + 0] = r;
    out[y * width + x * 3 + 1] = g;
    out[y * width + x * 3 + 2] = b;
  }
  ```

### 复盘问题

1. Denoiser 的输入和输出为什么都必须是 float4？uint8 不行吗？
2. Albedo 和 normal buffer 如何帮助 denoiser 更好地去噪？
3. 如果不提供 albedo/normal（设为 null），denoiser 的效果会差多少？
4. Tonemap 的目的是什么？为什么不直接将 float 乘以 255 转成 uint8？
5. ACES tonemap 与简单 gamma correction 相比的优势是什么？
6. 这题的 OptiX-CUDA interop 体现了什么设计哲学？

### 对应官方参考

- OptiX Programming Guide - Denoiser：https://raytracing-docs.nvidia.com/optix8/guide/index.html#denoiser
- OptiX SDK - denoising 示例
- ACES Tonemap 论文：https://github.com/ampas/aces-dev
- NVIDIA Blog - Real-Time Denoising with OptiX

---

## 做完模块 K 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- OptiX 8 的五大 program type（raygen / intersection / any-hit / closest-hit / miss）各自的职责和执行时机。
- Shader Binding Table（SBT）为什么采用三段式（raygen / miss / hitgroup），如何映射几何体 ID 到具体 shader。
- 为什么 OptiX kernel 需要用 `nvcc --optix-ir` 编译成 `.optixir` 中间格式，而不是普通 PTX。
- Acceleration Structure（GAS + IAS）与 BVH 的关系，如何加速光线求交。
- `optixTrace` 的 payload 机制如何实现 raygen 与 hit/miss program 的隐式参数通讯。
- OptiX Denoiser 的输入格式（float4 RGBA）、output buffer layout、与 CUDA kernel 的互操作边界。
- 一个完整的光线追踪管线从"场景定义"到"去噪输出"的端到端流程。

