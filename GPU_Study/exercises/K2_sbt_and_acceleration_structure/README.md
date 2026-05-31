# K2: SBT 三段式 + GAS 加速结构

在 K1 基础上引入真实场景几何：Cornell Box 三角形网格、
GAS（Geometry Acceleration Structure）和三段式 SBT（raygen / miss / hitgroup），
输出法线可视化图像 `output_k2.ppm`。

> 安装 OptiX SDK 的步骤请参考 **K1/README.md** 的"OptiX SDK 安装"一节。

---

## 练习目标

`[K2-T01]` (programs.cu:3) 练习 K2 OptiX 设备程序。
`[K2-T02]` (programs.cu:4) 空注释行。
`[K2-T03]` (programs.cu:5) program type 列表：
`[K2-T04]` (programs.cu:6) `__raygen__cam` — 生成透视光线，调用 `optixTrace`。
`[K2-T05]` (programs.cu:7) `__miss__background` — 返回天蓝色背景（通过 payload 传回 raygen）。
`[K2-T06]` (programs.cu:8) `__closesthit__radiance` — 计算三角形法线，写入法线可视化颜色。

`[K2-T60]` (host.cpp:3) 练习 K2：SBT 三段式 + GAS 加速结构。
`[K2-T61]` (host.cpp:4) 空注释行。
`[K2-T62]` (host.cpp:5) 目标：
`[K2-T63]` (host.cpp:6) 用三角形网格（Cornell Box）构建 GAS（Geometry Acceleration Structure）。
`[K2-T64]` (host.cpp:7) 同上：以 Cornell Box 为示例。
`[K2-T65]` (host.cpp:8) 三段式 SBT：raygen / miss / hitgroup。
`[K2-T66]` (host.cpp:9) closest-hit 计算法线并通过 payload 返回法线可视化颜色。
`[K2-T67]` (host.cpp:10) 输出 `output_k2.ppm`。
`[K2-T68]` (host.cpp:11) 空注释行。
`[K2-T69]` (host.cpp:12) 在 K1 基础上新增：GAS build + IAS（可选）+ miss/hitgroup program group。

- 用 `optixAccelBuild` 构建 GAS，表达 Cornell Box 三角形网格
- 用 `__miss__background` 返回渐变背景色
- 用 `__closesthit__radiance` 计算三角形法线并可视化
- 掌握 SBT hitgroup record 中携带 per-geometry 数据（顶点/索引指针）的方法

---

## 文件结构

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | 构建脚本，含 `.optixir` custom command |
| `host.cpp` | Host 侧驱动（GAS build + SBT 三段式 + launch）|
| `programs.cu` | 设备程序（raygen + miss + closest-hit）|
| `README.md` | 本文档 |

---

## 必做任务清单

`[K2-T07]` (programs.cu:15) 共享数据结构 — 与 host.cpp 保持完全一致。
`[K2-T08]` (programs.cu:32) payload 辅助（3 个 uint 分别携带 R/G/B float 位模式）。
`[K2-T09]` (programs.cu:48) `__raygen__cam`。
`[K2-T10]` (programs.cu:49) 空注释行。
`[K2-T11]` (programs.cu:50) 生成透视相机光线（相机位于 z=3，朝 -z 看，FOV 约 60 度）。
`[K2-T12]` (programs.cu:51) 同上：相机参数说明。
`[K2-T13]` (programs.cu:52) 调用 optixTrace，将 miss/closest-hit 返回的颜色写入 output buffer。
`[K2-T14]` (programs.cu:53) 空注释行。
`[K2-T15]` (programs.cu:54) TODO [必做] 步骤 10（对应文档 K2 第 10 项）：
`[K2-T16]` (programs.cu:55) 用 `(u, v)` 计算光线方向。
`[K2-T17]` (programs.cu:56) 初始化 payload（清零）。
`[K2-T18]` (programs.cu:57) 调用 `optixTrace(params.handle, origin, direction, ...)`。
`[K2-T19]` (programs.cu:58) 将 payload rgb 写入 `params.output`。
`[K2-T20]` (programs.cu:67) 透视相机参数。
`[K2-T21]` (programs.cu:68) 近平面左下角。
`[K2-T22]` (programs.cu:69) 近平面水平向量。
`[K2-T23]` (programs.cu:70) 近平面垂直向量。
`[K2-T24]` (programs.cu:72) TODO [必做] 计算光线方向。
`[K2-T25]` (programs.cu:83) normalize。
`[K2-T26]` (programs.cu:89) TODO [必做] 初始化 payload，避免读到未定义值。
`[K2-T27]` (programs.cu:92) TODO [必做] 调用 `optixTrace`。
`[K2-T28]` (programs.cu:96) tmin。
`[K2-T29]` (programs.cu:97) tmax。
`[K2-T30]` (programs.cu:98) rayTime。
`[K2-T31]` (programs.cu:101) SBT offset。
`[K2-T32]` (programs.cu:102) SBT stride。
`[K2-T33]` (programs.cu:103) miss SBT index。
`[K2-T34]` (programs.cu:112) TODO [必做] 写出结果（步骤 11）。
`[K2-T35]` (programs.cu:118) `__miss__background`。
`[K2-T36]` (programs.cu:119) 空注释行。
`[K2-T37]` (programs.cu:120) 光线未打到任何几何体时执行。
`[K2-T38]` (programs.cu:121) TODO [必做] 步骤 4：根据光线方向返回渐变背景色。
`[K2-T39]` (programs.cu:124) TODO [必做] 取光线方向计算天空渐变。
`[K2-T40]` (programs.cu:126) 简单：基于 y 分量线性混合天蓝色和白色。
`[K2-T41]` (programs.cu:135) `__closesthit__radiance`。
`[K2-T42]` (programs.cu:136) 空注释行。
`[K2-T43]` (programs.cu:137) 光线打到三角形面片时执行。
`[K2-T44]` (programs.cu:138) 从 SBT data 取顶点/索引缓冲，计算法线，写入法线可视化颜色。
`[K2-T45]` (programs.cu:139) 空注释行。
`[K2-T46]` (programs.cu:140) TODO [必做] 步骤 5（对应文档 K2 第 5 项）：
`[K2-T47]` (programs.cu:141) 用 `optixGetPrimitiveIndex()` 获取三角形 ID。
`[K2-T48]` (programs.cu:142) 从 HitGroupData 读取 vertices/indices。
`[K2-T49]` (programs.cu:143) 用 cross product 计算法线。
`[K2-T50]` (programs.cu:144) 将法线 * 0.5 + 0.5 写入 payload（法线可视化）。
`[K2-T51]` (programs.cu:147) TODO [必做] 获取 SBT per-record 数据。
`[K2-T52]` (programs.cu:151) TODO [必做] 获取三角形索引。
`[K2-T53]` (programs.cu:159) TODO [必做] 计算法线（cross product of two edges）。
`[K2-T54]` (programs.cu:170) 法线可视化：n * 0.5 + 0.5 映射到 [0, 1]。
`[K2-T55]` (programs.cu:176) TODO [进阶] 不同面返回不同材质颜色（修改 HitGroupData 加 color 字段）。

对应模块文档 K2 第 1-11 项：

- [ ] 步骤 1：定义 Cornell Box 顶点/索引，`cudaMemcpy` 上传到 device
- [ ] 步骤 2：`OptixBuildInputTriangleArray` + `optixAccelBuild` 构建 GAS
- [ ] 步骤 3：（可选）IAS 支持多 instance
- [ ] 步骤 4：`__miss__background` 写入 payload（背景色）
- [ ] 步骤 5：`__closesthit__radiance` 计算法线，写入法线可视化颜色
- [ ] 步骤 7a：raygen program group
- [ ] 步骤 7b：miss program group
- [ ] 步骤 7c：hitgroup program group（携带 closest-hit；无 any-hit/intersection）
- [ ] 步骤 8：SBT 三段式（raygen 1 + miss 1 + hitgroup 1）
- [ ] 步骤 9：`optixPipelineCreate`（payloadValues=3）
- [ ] 步骤 10：raygen 中 `optixTrace` 生成透视光线，接收颜色

---

## host.cpp 任务标记

`[K2-T72]` (host.cpp:43) `OPTIX_CHECK`。
`[K2-T73]` (host.cpp:62) 文件读取。
`[K2-T75]` (host.cpp:74) Params — 与 programs.cu 共享。
`[K2-T76]` (host.cpp:80) GAS / IAS traversable handle。
`[K2-T77]` (host.cpp:84) SBT record 模板。
`[K2-T78]` (host.cpp:95) device 端顶点数组指针（用于法线计算）。
`[K2-T79]` (host.cpp:96) device 端索引数组指针。
`[K2-T80]` (host.cpp:103) Cornell Box 几何数据（8 顶点，12 三角形）。
`[K2-T81]` (host.cpp:104) 坐标范围 `[-1, 1]^3`；相机从 z=3 向 -z 方向看。
`[K2-T82]` (host.cpp:107) TODO [必做] 步骤 1：定义 Cornell Box 顶点和索引。
`[K2-T83]` (host.cpp:108) 下面给出 8 顶点 + 12 面（每面 2 三角形）的框架，需补全坐标。
`[K2-T84]` (host.cpp:111) 后墙（z = -1）。
`[K2-T85]` (host.cpp:114) 前墙（z = 1，通常不可见，作为相机背面）。
`[K2-T86]` (host.cpp:120) 后墙（白）。
`[K2-T87]` (host.cpp:122) 地板（白）。
`[K2-T88]` (host.cpp:124) 天花板（白）。
`[K2-T89]` (host.cpp:126) 左墙（红）。
`[K2-T90]` (host.cpp:128) 右墙（蓝）。
`[K2-T91]` (host.cpp:130) TODO [必做] 补充前墙（可选，通常不加以免遮挡相机）。
`[K2-T92]` (host.cpp:135) 写出 PPM。
`[K2-T94]` (host.cpp:153) main。
`[K2-T96]` (host.cpp:161) CUDA context。
`[K2-T97]` (host.cpp:168) OptiX init。
`[K2-T98]` (host.cpp:178) TODO [必做] 步骤 1：上传几何数据到 device。
`[K2-T99]` (host.cpp:189) TODO [必做] 步骤 2：构建 GAS。
`[K2-T100]` (host.cpp:198) TODO [必做] 构造 `OptixBuildInputTriangleArray`。
`[K2-T101]` (host.cpp:218) TODO [必做] 查询 buffer 大小。
`[K2-T102]` (host.cpp:227) TODO [必做] `optixAccelBuild(...)`。
`[K2-T103]` (host.cpp:236) GAS 构建完成。
`[K2-T104]` (host.cpp:238) TODO [进阶] 使用 `optixAccelCompact` 压缩 GAS 节省显存。
`[K2-T105]` (host.cpp:241) 加载 .optixir，创建 module。
`[K2-T106]` (host.cpp:247) payload: R, G, B (3 uints)。
`[K2-T107]` (host.cpp:248) barycentrics。
`[K2-T108]` (host.cpp:266) TODO [必做] 步骤 7a：创建 raygen program group。
`[K2-T109]` (host.cpp:278) TODO [必做] 步骤 7b：创建 miss program group。
`[K2-T110]` (host.cpp:290) TODO [必做] 步骤 7c：创建 hitgroup program group。
`[K2-T111]` (host.cpp:297) K2 无 any-hit。
`[K2-T112]` (host.cpp:299) 三角形使用内置求交。
`[K2-T113]` (host.cpp:305) 3 个 program group 创建完成。
`[K2-T114]` (host.cpp:307) TODO [必做] 步骤 9：构建 pipeline。
`[K2-T115]` (host.cpp:318) pipeline 创建完成。
`[K2-T116]` (host.cpp:321) output buffer。
`[K2-T117]` (host.cpp:326) Params。
`[K2-T118]` (host.cpp:337) TODO [必做] 步骤 8：构造三段式 SBT。
`[K2-T119]` (host.cpp:338) raygen 段（1 record）。
`[K2-T120]` (host.cpp:345) miss 段（1 record）。
`[K2-T121]` (host.cpp:348) 天蓝色背景。
`[K2-T122]` (host.cpp:353) hitgroup 段（1 record — Cornell Box 所有面共用同一 closest-hit）。
`[K2-T123]` (host.cpp:370) SBT 三段式构造完成。
`[K2-T124]` (host.cpp:372) optixLaunch。
`[K2-T125]` (host.cpp:385) optixLaunch 完成。
`[K2-T126]` (host.cpp:387) 读回并写出图像。
`[K2-T127]` (host.cpp:393) TODO [进阶] 添加第二个 closest-hit program（镜面 vs Lambertian）。
`[K2-T128]` (host.cpp:394) TODO [进阶] 多 instance + 不同变换矩阵（IAS）。
`[K2-T129]` (host.cpp:395) TODO [进阶] 在 closest-hit 中递归 optixTrace 计算反射光线。
`[K2-T130]` (host.cpp:397) 清理。

---

## 进阶任务

- 添加第二个 hitgroup record（不同颜色区分各墙面）
- 支持多 instance + 不同变换矩阵（IAS）
- 在 closest-hit 中递归 `optixTrace` 计算反射光线
- 调用 `optixAccelCompact` 压缩 GAS

---

## 验收点

- 编译无错误，`programs.optixir` 生成
- 运行无 OptiX API error
- `output_k2.ppm` 显示 Cornell Box 法线可视化（各墙面颜色可区分）
- Nsight Compute 可见 `__closesthit__radiance` 被调用

---

## 常见问题

**Q: GAS build 返回 `OPTIX_ERROR_INVALID_INPUT`**
A: 检查 `vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3`，以及 `indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3`。

**Q: SBT hitgroup record 大小不对齐**
A: 使用 `SbtRecord<T>` 模板加 `alignas(OPTIX_SBT_RECORD_ALIGNMENT)`，确保总大小是 16 的倍数。

**Q: 法线方向反了（颜色偏暗）**
A: 检查 cross product 的顶点顺序（e1 = v1-v0, e2 = v2-v0）。
   若法线朝内，交换 e1/e2 或乘以 -1。

**Q: miss 程序未被调用（背景是黑色）**
A: 确认 SBT 的 `missRecordBase` 已设置，且 `optixTrace` 的 miss SBT index 参数为 0。

---

## 参考资料

- OptiX 8 Guide - Acceleration Structures:
  https://raytracing-docs.nvidia.com/optix8/guide/index.html#acceleration_structures
- OptiX 8 Guide - Shader Binding Table:
  https://raytracing-docs.nvidia.com/optix8/guide/index.html#shader_binding_table

---

## 输出对照（printf / std::puts 原文）

- `[K2-T70]` (host.cpp:20) 原文：`[K2] GPU_STUDY_NO_OPTIX=1: OptiX SDK 未安装。` → 现：`[K2] GPU_STUDY_NO_OPTIX=1: OptiX SDK is not installed.`
- `[K2-T71]` (host.cpp:22) 原文：`     请参考 K1/README.md 完成安装后重新配置 CMake。` → 现：`     See K1/README.md and re-configure CMake after installation.`
- `[K2-T74]` (host.cpp:67) 原文：`无法打开: %s\n` → 现：`Cannot open: %s\n`
- `[K2-T93]` (host.cpp:148) 原文：`  输出已写入: %s\n` → 现：`  Output written: %s\n`
- `[K2-T95]` (host.cpp:155) 原文：`[K2] sbt_and_acceleration_structure` → 现：保持英文不变
- `[K2-T103]` (host.cpp:236) 原文：`  GAS 构建完成` → 现：`  GAS build complete`
- `[K2-T113]` (host.cpp:305) 原文：`  3 个 program group 创建完成` → 现：`  3 program groups created`
- `[K2-T115]` (host.cpp:318) 原文：`  pipeline 创建完成` → 现：`  pipeline created`
- `[K2-T123]` (host.cpp:370) 原文：`  SBT 三段式构造完成` → 现：`  SBT three segments built`
- `[K2-T125]` (host.cpp:385) 原文：`  optixLaunch 完成` → 现：`  optixLaunch finished`
- `[K2-T131]` (host.cpp:415) 原文：`[K2] 完成。` → 现：`[K2] done.`
