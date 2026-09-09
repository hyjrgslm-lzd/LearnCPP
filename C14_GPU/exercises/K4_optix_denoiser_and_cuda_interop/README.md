# K4: OptiX Denoiser + CUDA Interop

在 K3 基础上引入蒙特卡罗路径追踪（32 spp）+ OptiX AI 去噪器 + CPU/CUDA Tonemap，
演示 OptiX 与 CUDA kernel 的互操作边界。
输出两个 PPM：`output_k4_noisy.ppm`（高噪声）和 `output_k4_denoised.ppm`（去噪）。

> 安装 OptiX SDK 的步骤请参考 **K1/README.md** 的"OptiX SDK 安装"一节。

---

## 练习目标

`[K4-T01]` (programs.cu:3) 练习 K4 OptiX 设备程序 — 蒙特卡罗路径追踪。
`[K4-T02]` (programs.cu:4) 同上：以及 albedo/normal guide buffer。
`[K4-T03]` (programs.cu:5) 空注释行。
`[K4-T04]` (programs.cu:6) program type 列表：
`[K4-T05]` (programs.cu:7) `__raygen__mc` — 32 spp MC 路径追踪（随机方向，累积平均）。
`[K4-T06]` (programs.cu:8) `__miss__background` — 背景色（depth=0）或阴影标记（depth=1）。
`[K4-T07]` (programs.cu:9) `__closesthit__radiance` — Lambertian + 写出 albedo / normal buffer。

`[K4-T50]` (host.cpp:3) 练习 K4：OptiX Denoiser + CUDA Interop。
`[K4-T51]` (host.cpp:4) 空注释行。
`[K4-T52]` (host.cpp:5) 目标：
`[K4-T53]` (host.cpp:6) 用 32 spp 蒙特卡罗路径追踪生成高噪声图像。
`[K4-T54]` (host.cpp:7) 用 OptiX Denoiser（AI 去噪）平滑噪声。
`[K4-T55]` (host.cpp:8) 用 CUDA kernel（kernel_tonemap_aces）将 HDR float4 转为 LDR uint8。
`[K4-T56]` (host.cpp:9) 同上：CUDA kernel 完成 tonemap。
`[K4-T57]` (host.cpp:10) 输出两个 PPM：`output_k4_noisy.ppm`（原始）。
`[K4-T58]` (host.cpp:11) 同上：再加 `output_k4_denoised.ppm`（去噪）。
`[K4-T59]` (host.cpp:12) 空注释行。
`[K4-T60]` (host.cpp:13) 新增内容（相对 K3）：
`[K4-T61]` (host.cpp:14) `optixDenoiserCreate / optixDenoiserSetup / optixDenoiserInvoke`。
`[K4-T62]` (host.cpp:15) albedo / normal guide buffer（可选，提升去噪质量）。
`[K4-T63]` (host.cpp:16) CUDA tonemap kernel（在 host.cpp 中以 `__global__` 声明需改为 .cu。
`[K4-T64]` (host.cpp:17) 同上：或通过运行时 API 调用；本题将 tonemap 定义在同文件底部）。
`[K4-T65]` (host.cpp:18) 同上：本题处理方式说明。
`[K4-T66]` (host.cpp:19) 空注释行。
`[K4-T67]` (host.cpp:20) 注意：host.cpp 是纯 C++ 编译单元（.cpp），不包含 `__global__` kernel。
`[K4-T68]` (host.cpp:21) CUDA tonemap kernel 在编译时须通过 nvcc 处理，因此本文件。
`[K4-T69]` (host.cpp:22) 使用 `cudaLaunchKernel + 单独的 .cu 文件`。
`[K4-T70]` (host.cpp:23) 同上：备选方案说明。
`[K4-T71]` (host.cpp:24) 本骨架采用简化方案：使用 CPU 端 tonemap（无需额外 .cu 文件）。
`[K4-T72]` (host.cpp:25) TODO [进阶] 将 tonemap 移入独立 .cu 文件并用 CUDA kernel 加速。
`[K4-T73]` (host.cpp:26) 同上：进阶任务延伸。

- 用 32 spp 蒙特卡罗路径追踪生成高噪声图像（raygen 循环采样）
- 分配 color / albedo / normal 三个 float4 guide buffer
- 用 `optixDenoiserCreate` + `optixDenoiserSetup` + `optixDenoiserInvoke` 去噪
- 用 ACES tonemap（CPU 端骨架，进阶改为 CUDA kernel）输出 LDR 图像
- 对比去噪前后的视觉差异

---

## 文件结构

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | 构建脚本 |
| `host.cpp` | Host 侧驱动（Denoiser + tonemap + 图像输出）|
| `programs.cu` | MC raygen + 写出 albedo/normal guide buffer |
| `README.md` | 本文档 |

---

## programs.cu 任务标记

`[K4-T08]` (programs.cu:16) 共享数据结构（与 host.cpp 保持完全一致）。
`[K4-T09]` (programs.cu:43) 简单 LCG 随机数生成（设备端）。
`[K4-T10]` (programs.cu:54) float3 辅助。
`[K4-T11]` (programs.cu:69) payload 辅助（4 slots: R, G, B, depth）。
`[K4-T12]` (programs.cu:84) `__raygen__mc`。
`[K4-T13]` (programs.cu:85) 空注释行。
`[K4-T14]` (programs.cu:86) TODO [必做] 步骤 1：
`[K4-T15]` (programs.cu:87) 外层循环 samples_per_pixel 次。
`[K4-T16]` (programs.cu:88) 每次生成随机抖动的像素坐标（stratified jitter）。
`[K4-T17]` (programs.cu:89) 调用 `optixTrace`（depth=0）。
`[K4-T18]` (programs.cu:90) 累积颜色，最后除以 spp 写入 color_buffer。
`[K4-T19]` (programs.cu:99) 初始化随机状态（像素 ID + seed 扰动）。
`[K4-T20]` (programs.cu:108) TODO [必做] 蒙特卡罗采样循环。
`[K4-T21]` (programs.cu:110) 像素内随机抖动（stratified jitter）。
`[K4-T22]` (programs.cu:126) depth = 0。
`[K4-T23]` (programs.cu:142) 平均 spp 次采样。
`[K4-T24]` (programs.cu:150) TODO [进阶] 实现自适应采样（高方差区域多采样）。
`[K4-T25]` (programs.cu:153) `__miss__background`。
`[K4-T26]` (programs.cu:167) 阴影 miss → 未遮挡标记。
`[K4-T27]` (programs.cu:171) `__closesthit__radiance`。
`[K4-T28]` (programs.cu:172) 空注释行。
`[K4-T29]` (programs.cu:173) TODO [必做] 步骤 4：
`[K4-T30]` (programs.cu:174) 计算法线 → 写入 normal_buffer（float4，xyz = 法线，w = 1）。
`[K4-T31]` (programs.cu:175) 取 base_color → 写入 albedo_buffer（float4，xyz = albedo，w = 1）。
`[K4-T32]` (programs.cu:176) 计算 Lambertian + 阴影光线（沿用 K3 逻辑）。
`[K4-T33]` (programs.cu:202) TODO [必做] 步骤 4：写入 albedo / normal guide buffer。
`[K4-T34]` (programs.cu:215) Lambertian + 阴影。
`[K4-T35]` (programs.cu:244) TODO [进阶] 实现多次反弹路径追踪（递归 optixTrace）。
`[K4-T36]` (programs.cu:245) TODO [进阶] 重要性采样半球（替换均匀随机方向）。

---

## 必做任务清单

对应模块文档 K4 第 1-10 项：

- [ ] 步骤 1：`__raygen__mc` 循环 32 次，累积颜色平均
- [ ] 步骤 2：`optixDenoiserCreate`（`MODEL_KIND_HDR`）
- [ ] 步骤 3：`OptixDenoiserParams`（`blendFactor = 0`）
- [ ] 步骤 4：`closest-hit` 写出 albedo / normal buffer
- [ ] 步骤 5：`cudaMalloc` 三个 float4 buffer（color + albedo + normal）
- [ ] 步骤 6：`optixDenoiserComputeMemoryResources` + `optixDenoiserSetup`
- [ ] 步骤 7：`optixDenoiserInvoke`（传入 guide_layer + dn_layer）
- [ ] 步骤 8：CPU ACES tonemap（或 CUDA kernel）
- [ ] 步骤 9：读回 `d_denoised`，写出 `output_k4_denoised.ppm`
- [ ] 步骤 10：同时保存 `output_k4_noisy.ppm` 作为对比基准

---

## 关键设计说明

### Denoiser Buffer 格式

OptiX Denoiser 要求输入/输出均为 `OPTIX_PIXEL_FORMAT_FLOAT4`（RGBA float32）。
`uint8` 格式会导致 `optixDenoiserInvoke` 返回错误。

### Guide Buffer 作用

| Buffer | 作用 |
|--------|------|
| color  | 主输入（有噪声的 HDR 图像）|
| albedo | 表面固有颜色，帮助 denoiser 识别材质边界 |
| normal | 法线方向，帮助 denoiser 保留几何细节 |

即使不使用 guide buffer（`guideAlbedo = 0`），denoiser 仍可工作，
但效果略差（边缘可能被过度平滑）。

### ACES Tonemap 公式

```
f(x) = (x * (a*x + b)) / (x * (c*x + d) + e)
a=2.51, b=0.03, c=2.43, d=0.59, e=0.14
```

后跟 gamma = 2.2 校正：`output = pow(f(x), 1/2.2)`

不要直接乘以 255——HDR 值可能 > 1，会 overflow。

### 进阶：CUDA Tonemap Kernel

将 CPU 端 tonemap 移入 CUDA kernel（见 K4 模块文档提示）：

```cuda
__global__ void kernel_tonemap_aces(float4* in, uint8_t* out, int w, int h) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= w || y >= h) return;
    // ... ACES + gamma ...
}
```

---

## host.cpp 任务标记

`[K4-T76]` (host.cpp:42) OptiX 8 denoiser 工具。
`[K4-T77]` (host.cpp:54) `OPTIX_CHECK`。
`[K4-T79]` (host.cpp:81) 共享数据结构（与 programs.cu 一致）。
`[K4-T80]` (host.cpp:88) 主颜色输出（float4 HDR）。
`[K4-T81]` (host.cpp:89) albedo guide（可选）。
`[K4-T82]` (host.cpp:90) normal guide（可选）。
`[K4-T83]` (host.cpp:95) MC 采样数。
`[K4-T84]` (host.cpp:96) 随机种子基础值。
`[K4-T85]` (host.cpp:99) SBT record 模板。
`[K4-T86]` (host.cpp:120) Cornell Box 几何（沿用 K2/K3）。
`[K4-T87]` (host.cpp:141) CPU 端 ACES tonemap（暂时用 CPU 实现；进阶改为 CUDA kernel）。
`[K4-T88]` (host.cpp:154) 写出 PPM（float4 HDR）。
`[K4-T90]` (host.cpp:171) main。
`[K4-T92]` (host.cpp:178) 蒙特卡罗采样数（故意低，产生明显噪声）。
`[K4-T93]` (host.cpp:180) CUDA context。
`[K4-T94]` (host.cpp:187) OptiX init。
`[K4-T95]` (host.cpp:197) 上传几何。
`[K4-T96]` (host.cpp:206) GAS 构建。
`[K4-T97]` (host.cpp:248) GAS 构建完成。
`[K4-T98]` (host.cpp:251) 加载 .optixir，创建 module。
`[K4-T99]` (host.cpp:275) Program groups。
`[K4-T100]` (host.cpp:312) pipeline。
`[K4-T101]` (host.cpp:323) pipeline 创建完成。
`[K4-T102]` (host.cpp:325) TODO [必做] 步骤 5：分配 color / albedo / normal buffer。
`[K4-T103]` (host.cpp:333) 初始化 albedo/normal（默认全白/法线朝上）。
`[K4-T104]` (host.cpp:334) 后续由 closest-hit 填写。
`[K4-T105]` (host.cpp:337) Params。
`[K4-T106]` (host.cpp:354) SBT。
`[K4-T107]` (host.cpp:387) TODO [必做] 步骤 1：`optixLaunch`（32 spp 路径追踪）。
`[K4-T108]` (host.cpp:402) 路径追踪完成（32 spp 高噪声）。
`[K4-T109]` (host.cpp:404) 读回并保存噪声原图。
`[K4-T110]` (host.cpp:411) TODO [必做] 步骤 2-7：OptiX Denoiser。
`[K4-T111]` (host.cpp:414) TODO [必做] 步骤 2：`optixDenoiserCreate`。
`[K4-T112]` (host.cpp:416) 使用 albedo guide。
`[K4-T113]` (host.cpp:417) 使用 normal guide。
`[K4-T114]` (host.cpp:425) TODO [必做] 步骤 6：查询并分配 state / scratch buffer。
`[K4-T115]` (host.cpp:444) Denoiser setup 完成。
`[K4-T116]` (host.cpp:446) TODO [必做] 步骤 3：设置 denoiser 参数。
`[K4-T117]` (host.cpp:448) `0 = 完全相信 denoiser 输出`。
`[K4-T118]` (host.cpp:449) 不使用 intensity pre-computation。
`[K4-T119]` (host.cpp:451) 分配 denoised 输出 buffer。
`[K4-T120]` (host.cpp:455) TODO [必做] 步骤 4：设置 albedo / normal guide buffer（可选）。
`[K4-T121]` (host.cpp:471) input layer（color）。
`[K4-T122]` (host.cpp:486) TODO [必做] 步骤 7：`optixDenoiserInvoke`。
`[K4-T123]` (host.cpp:495) input offset x/y。
`[K4-T124]` (host.cpp:499) Denoiser invoke 完成。
`[K4-T125]` (host.cpp:501) TODO [必做] 步骤 8-9：读回去噪结果并写出。
`[K4-T126]` (host.cpp:506) TODO [必做] 步骤 8：tonemap + 写出 `output_k4_denoised.ppm`。
`[K4-T127]` (host.cpp:509) TODO [进阶] 将 tonemap 移入独立 CUDA kernel。
`[K4-T128]` (host.cpp:510) TODO [进阶] 比较 `blendFactor = 0.0 / 0.5 / 1.0` 的去噪效果差异。
`[K4-T129]` (host.cpp:511) TODO [进阶] albedo-guided + normal-guided denoising 效果对比。
`[K4-T130]` (host.cpp:519) 清理。

---

## 进阶任务

- 将 CPU tonemap 改为 CUDA kernel（`kernel_tonemap_aces<<<grid, block>>>`）
- 对比 `blendFactor = 0.0 / 0.5 / 1.0` 的视觉差异
- 实现 albedo-guided + normal-guided denoising 效果对比（设 `guideAlbedo=0` 关闭）
- 实现多帧累积（temporal denoising）

---

## 验收点

- 编译无错误，`programs.optixir` 生成
- 运行无 OptiX API error 或 CUDA error
- `output_k4_noisy.ppm`：可见明显噪点（颗粒感）
- `output_k4_denoised.ppm`：平滑，保留 Cornell Box 几何细节
- Tonemap 结果无 overflow（无纯白色块）或 underflow（无全黑）

---

## 常见问题

**Q: `optixDenoiserCreate` 返回 `OPTIX_ERROR_INVALID_VALUE`**
A: 检查 `OptixDenoiserOptions.guideAlbedo/guideNormal` 设置是否与
   `optixDenoiserInvoke` 传入的 `guide_layer` 一致。
   若声明 `guideAlbedo=1` 但 `guide_layer.albedo.data = 0`，会报错。

**Q: 去噪后图像是黑色**
A: `optixDenoiserInvoke` 之后需要 `cudaStreamSynchronize`，
   否则 `cudaMemcpy` 读到未完成的结果。

**Q: 噪声原图很暗**
A: `closest-hit` 的 `visibility` 计算问题，或光强过低。
   尝试增大 `params.light.intensity`（如 5.0）。

**Q: Denoiser 报分辨率不支持**
A: 某些版本要求宽高是 8 或 16 的倍数。1024×1024 满足此要求；
   如果改了分辨率请确保满足对齐要求。

---

## 参考资料

- OptiX 8 Guide - Denoiser:
  https://raytracing-docs.nvidia.com/optix8/guide/index.html#denoiser
- ACES Tonemap: https://github.com/ampas/aces-dev
- OptiX SDK denoising 示例: `<SDK_ROOT>/SDK/optixDenoiser/`

---

## 输出对照（printf / std::puts 原文）

- `[K4-T74]` (host.cpp:30) 原文：`[K4] GPU_STUDY_NO_OPTIX=1: OptiX SDK 未安装。` → 现：`[K4] GPU_STUDY_NO_OPTIX=1: OptiX SDK is not installed.`
- `[K4-T75]` (host.cpp:32) 原文：`     请参考 K1/README.md 完成安装后重新配置 CMake。` → 现：`     See K1/README.md and re-configure CMake after installation.`
- `[K4-T78]` (host.cpp:74) 原文：`无法打开: %s\n` → 现：`Cannot open: %s\n`
- `[K4-T89]` (host.cpp:166) 原文：`  输出已写入: %s\n` → 现：`  Output written: %s\n`
- `[K4-T91]` (host.cpp:173) 原文：`[K4] optix_denoiser_and_cuda_interop` → 现：保持英文不变
- `[K4-T97]` (host.cpp:248) 原文：`  GAS 构建完成` → 现：`  GAS build complete`
- `[K4-T101]` (host.cpp:323) 原文：`  pipeline 创建完成` → 现：`  pipeline created`
- `[K4-T108]` (host.cpp:402) 原文：`  路径追踪完成（32 spp 高噪声）` → 现：`  Path tracing complete (32 spp, high noise)`
- `[K4-T115]` (host.cpp:444) 原文：`  Denoiser setup 完成` → 现：`  Denoiser setup complete`
- `[K4-T124]` (host.cpp:499) 原文：`  Denoiser invoke 完成` → 现：`  Denoiser invoke complete`
- `[K4-T131]` (host.cpp:537) 原文：`[K4] 完成。` → 现：`[K4] done.`
