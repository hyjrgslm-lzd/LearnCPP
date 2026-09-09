# K3: intersection / any-hit / closest-hit / miss — 五大 program type

在 K2 基础上，K3 完整展示 OptiX 五大 program type 的分工。
Cornell Box 增加点光源 + Lambertian 直接照明 + 阴影光线，
输出 `output_k3.ppm`。

> 安装 OptiX SDK 的步骤请参考 **K1/README.md** 的"OptiX SDK 安装"一节。

---

## 练习目标

`[K3-T01]` (programs.cu:3) 练习 K3 OptiX 设备程序 — 五大 program type。
`[K3-T02]` (programs.cu:4) 空注释行。
`[K3-T03]` (programs.cu:5) program type 列表：
`[K3-T04]` (programs.cu:6) `__raygen__principal` — 生成初级光线（depth=0）。
`[K3-T05]` (programs.cu:7) `__miss__background` — 检查 depth，返回背景色或阴影标记。
`[K3-T06]` (programs.cu:8) `__anyhit__opaque` — alpha 测试占位符（全部通过）。
`[K3-T07]` (programs.cu:9) `__closesthit__radiance` — Lambertian 直接照明 + 阴影光线。
`[K3-T08]` (programs.cu:10) 空注释行。
`[K3-T09]` (programs.cu:11) 进阶（骨架，默认注释）：
`[K3-T10]` (programs.cu:12) `__intersection__sphere` — 自定义球求交。

`[K3-T80]` (host.cpp:3) 练习 K3：五大 program type 完整展示。
`[K3-T81]` (host.cpp:4) 空注释行。
`[K3-T82]` (host.cpp:5) 目标：
`[K3-T83]` (host.cpp:6) raygen：生成初级光线（depth=0）。
`[K3-T84]` (host.cpp:7) any-hit：透明度/alpha 测试骨架。
`[K3-T85]` (host.cpp:8) closest-hit：Lambertian 直接照明 + 阴影光线。
`[K3-T86]` (host.cpp:9) miss：检查 depth，返回背景色或遮挡标记。
`[K3-T87]` (host.cpp:10) intersection：自定义球求交（进阶，骨架已准备）。
`[K3-T88]` (host.cpp:11) 空注释行。
`[K3-T89]` (host.cpp:12) 在 K2 基础上新增：
`[K3-T90]` (host.cpp:13) PointLight 常量。
`[K3-T91]` (host.cpp:14) RayPayload（rgb + depth）。
`[K3-T92]` (host.cpp:15) any-hit program group。
`[K3-T93]` (host.cpp:16) 阴影光线 payload。

- `__raygen__principal`：生成初级光线（depth=0），初始化 4-uint payload
- `__miss__background`：按 depth 区分初级/阴影 miss，返回不同结果
- `__anyhit__opaque`：alpha 测试骨架（全部通过，为透明物体留接口）
- `__closesthit__radiance`：Lambertian 直接照明 + 阴影光线递归
- 自定义 intersection program 球求交（进阶骨架）

---

## 文件结构

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | 构建脚本 |
| `host.cpp` | Host 侧驱动（每三角形一条 hitgroup SBT record）|
| `programs.cu` | 五大 program type 设备代码 |
| `README.md` | 本文档 |

---

## programs.cu 任务标记

`[K3-T11]` (programs.cu:19) 共享数据结构 — 与 host.cpp 保持完全一致。
`[K3-T12]` (programs.cu:42) payload 辅助。
`[K3-T13]` (programs.cu:43) `payload[0..2] = R/G/B (float bit-cast)`。
`[K3-T14]` (programs.cu:44) `payload[3] = depth (uint: 0=初级, 1=阴影)`。
`[K3-T15]` (programs.cu:65) float3 辅助。
`[K3-T16]` (programs.cu:78) `__raygen__principal`。
`[K3-T17]` (programs.cu:79) 空注释行。
`[K3-T18]` (programs.cu:80) TODO [必做] 步骤 7：
`[K3-T19]` (programs.cu:81) 设置 payload.depth = 0。
`[K3-T20]` (programs.cu:82) 计算透视光线方向。
`[K3-T21]` (programs.cu:83) 初始化 payload 清零。
`[K3-T22]` (programs.cu:84) 调用 `optixTrace`（4 个 payload 槽位）。
`[K3-T23]` (programs.cu:85) 将 payload rgb 写入 output buffer。
`[K3-T24]` (programs.cu:108) TODO [必做] 初始化 payload（深度 = 0，颜色清零）。
`[K3-T25]` (programs.cu:110) depth = 0（初级光线）。
`[K3-T26]` (programs.cu:112) TODO [必做] 调用 `optixTrace`（4 个 payload 槽）。
`[K3-T27]` (programs.cu:128) `__miss__background`。
`[K3-T28]` (programs.cu:129) 空注释行。
`[K3-T29]` (programs.cu:130) TODO [必做] 步骤 6：
`[K3-T30]` (programs.cu:131) 检查 payload depth。
`[K3-T31]` (programs.cu:132) depth == 0（初级光线）：返回渐变背景色。
`[K3-T32]` (programs.cu:133) depth == 1（阴影光线）：返回 (1,1,1) 表示未遮挡（可见光源）。
`[K3-T33]` (programs.cu:139) 初级光线 miss → 背景天蓝色渐变。
`[K3-T34]` (programs.cu:148) 阴影光线 miss → 光源可见，返回 "未遮挡" 标记。
`[K3-T35]` (programs.cu:149) closest-hit 中用 depth=2 表示未遮挡。
`[K3-T36]` (programs.cu:154) `__anyhit__opaque`。
`[K3-T37]` (programs.cu:155) 空注释行。
`[K3-T38]` (programs.cu:156) TODO [必做] 步骤 2：alpha 测试占位符。
`[K3-T39]` (programs.cu:157) 此实现全部通过（alpha=1.0，不透明物体）。
`[K3-T40]` (programs.cu:158) 如需透明，调用 `optixIgnoreIntersection()` 跳过当前交点。
`[K3-T41]` (programs.cu:163) TODO [必做] 全部通过（不透明）。
`[K3-T42]` (programs.cu:164) 如需 alpha 测试示例。
`[K3-T43]` (programs.cu:167) 此处无需任何操作（OptiX 默认保留当前交点）。
`[K3-T44]` (programs.cu:170) `__closesthit__radiance`。
`[K3-T45]` (programs.cu:171) 空注释行。
`[K3-T46]` (programs.cu:172) TODO [必做] 步骤 4：Lambertian 直接照明。
`[K3-T47]` (programs.cu:173) 1. 从 HitGroupData 取顶点/索引，计算法线。
`[K3-T48]` (programs.cu:174) 2. 计算 hit point（光线起点 + t * 方向）。
`[K3-T49]` (programs.cu:175) 3. 生成阴影光线（depth=1），检查遮挡。
`[K3-T50]` (programs.cu:176) 4. 计算 Lambertian cosine term。
`[K3-T51]` (programs.cu:177) 5. 若未遮挡：color = base_color * intensity * cosine。
`[K3-T52]` (programs.cu:178) 若遮挡：color = 0（阴影）。
`[K3-T53]` (programs.cu:194) TODO [必做] 计算法线。
`[K3-T54]` (programs.cu:199) TODO [必做] 计算 hit point（加法线偏移防自相交）。
`[K3-T55]` (programs.cu:209) TODO [必做] 计算光源方向 + cosine。
`[K3-T56]` (programs.cu:220) TODO [必做] 生成阴影光线（depth=1）。
`[K3-T57]` (programs.cu:222) depth = 1（阴影光线）。
`[K3-T58]` (programs.cu:227) tmin（防止自相交）。
`[K3-T59]` (programs.cu:228) tmax（不超过光源位置）。
`[K3-T60]` (programs.cu:235) `sp3 == 2` 表示阴影光线 miss（光源可见）；否则 hit 某面（被遮挡）。
`[K3-T61]` (programs.cu:238) TODO [必做] Lambertian 着色。
`[K3-T62]` (programs.cu:246) TODO [进阶] Phong 镜面反射项。
`[K3-T63]` (programs.cu:247) TODO [进阶] 多光源循环累加。
`[K3-T64]` (programs.cu:251) `__intersection__sphere`（进阶骨架，默认未启用）。
`[K3-T65]` (programs.cu:252) 空注释行。
`[K3-T66]` (programs.cu:253) 当 CMakeLists.txt 中启用自定义 primitive 时。
`[K3-T67]` (programs.cu:254) 需在 host 侧添加 `OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES` 的 GAS。
`[K3-T68]` (programs.cu:255) 空注释行。
`[K3-T69]` (programs.cu:256) TODO [进阶] 步骤 3：计算射线与球的交点 t，调用 `optixReportIntersection`。
`[K3-T70]` (programs.cu:257) 同上：进阶任务说明。

---

## 必做任务清单

对应模块文档 K3 第 1-9 项：

- [ ] 步骤 1：`PointLight` struct + 在 `Params` 中传递
- [ ] 步骤 2：`__anyhit__opaque` 全部通过（骨架验证编译）
- [ ] 步骤 4：`__closesthit__radiance` — 计算 cosine term + 阴影光线
- [ ] 步骤 5：payload 扩展为 4 uints（rgb + depth）
- [ ] 步骤 6：`__miss__background` 检查 depth
- [ ] 步骤 7：`__raygen__principal` 设置 depth=0，4-slot optixTrace
- [ ] 步骤 9：hitgroup program group 含 any-hit + closest-hit
- [ ] 步骤 9b：`maxTraceDepth = 2`（初级 + 阴影）
- [ ] 步骤 9c：`numSbtRecords = NUM_TRIANGLES`（每面独立 record）

---

## 关键设计说明

### Payload 布局

```
payload[0] = R  (float bit-cast 到 uint)
payload[1] = G  (float bit-cast 到 uint)
payload[2] = B  (float bit-cast 到 uint)
payload[3] = depth (uint: 0=初级, 1=阴影, 2=阴影-miss标记)
```

Pipeline `numPayloadValues = 4`，必须与 `optixTrace` 调用的槽位数一致。

### 阴影光线约定

`__closesthit__radiance` 生成阴影光线时：
- 设置 `p3 = 1`（depth=1，表示阴影光线）
- `tmax = light_dist - epsilon`（不超过光源）
- 使用 `OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT | OPTIX_RAY_FLAG_DISABLE_CLOSESTHIT`
- `__miss__background` 检测到 depth=1 时设 `p3 = 2`（未遮挡标记）
- closest-hit 检查 `sp3 == 2` 决定可见性

### Hit point 偏移

```cpp
hit_point = ray_orig + t * ray_dir + normal * 1e-4f
```

不加偏移会导致 shadow ray 立即与当前面自相交，产生错误阴影。

---

## host.cpp 任务标记

`[K3-T96]` (host.cpp:46) `OPTIX_CHECK`。
`[K3-T98]` (host.cpp:74) 共享数据结构（与 programs.cu 一致）。
`[K3-T99]` (host.cpp:78) TODO [必做] 步骤 1：`PointLight` 定义。
`[K3-T100]` (host.cpp:80) 世界坐标。
`[K3-T101]` (host.cpp:81) 每通道光强（线性 HDR）。
`[K3-T102]` (host.cpp:89) 单点光源。
`[K3-T103]` (host.cpp:93) SBT record 模板。
`[K3-T104]` (host.cpp:106) Lambertian 材质固有色。
`[K3-T105]` (host.cpp:113) Cornell Box 几何数据（沿用 K2）。
`[K3-T106]` (host.cpp:122) 后墙（白）。
`[K3-T107]` (host.cpp:123) 地板（白）。
`[K3-T108]` (host.cpp:124) 天花板（白）。
`[K3-T109]` (host.cpp:125) 左墙（红）。
`[K3-T110]` (host.cpp:126) 右墙（蓝）。
`[K3-T111]` (host.cpp:131) 与三角形顺序对应的材质颜色（5 组面各 2 三角形）。
`[K3-T112]` (host.cpp:133) 后墙白。
`[K3-T113]` (host.cpp:134) 地板白。
`[K3-T114]` (host.cpp:135) 天花白。
`[K3-T115]` (host.cpp:136) 左墙红。
`[K3-T116]` (host.cpp:137) 右墙蓝。
`[K3-T117]` (host.cpp:140) 写出 PPM。
`[K3-T119]` (host.cpp:158) main。
`[K3-T121]` (host.cpp:166) CUDA context。
`[K3-T122]` (host.cpp:173) OptiX init。
`[K3-T123]` (host.cpp:182) 上传几何数据。
`[K3-T124]` (host.cpp:191) GAS 构建。
`[K3-T125]` (host.cpp:212) 每个三角形独立 SBT record（用于区分材质颜色）。
`[K3-T126]` (host.cpp:222) 每三角形 1 record。
`[K3-T127]` (host.cpp:236) GAS 构建完成。
`[K3-T128]` (host.cpp:239) 加载 .optixir，创建 module。
`[K3-T129]` (host.cpp:242) TODO [必做] 步骤 5：`payloadValues = 4`（rgb 3 uints + depth 1 uint）。
`[K3-T130]` (host.cpp:246) R, G, B, depth。
`[K3-T131]` (host.cpp:247) barycentrics。
`[K3-T132]` (host.cpp:265) Program groups。
`[K3-T133]` (host.cpp:269) raygen。
`[K3-T134]` (host.cpp:280) miss。
`[K3-T135]` (host.cpp:291) TODO [必做] 步骤 9：hitgroup 包含 any-hit + closest-hit。
`[K3-T136]` (host.cpp:299) 三角形内置求交。
`[K3-T137]` (host.cpp:307) program groups 创建完成。
`[K3-T138]` (host.cpp:309) pipeline。
`[K3-T139]` (host.cpp:311) TODO [必做] 步骤 9b：`maxTraceDepth = 2`（初级光线 + 阴影光线）。
`[K3-T140]` (host.cpp:321) pipeline 创建完成。
`[K3-T141]` (host.cpp:324) output buffer。
`[K3-T142]` (host.cpp:329) Params。
`[K3-T143]` (host.cpp:335) TODO [必做] 步骤 1：设置点光源位置和强度。
`[K3-T144]` (host.cpp:336) 天花板附近。
`[K3-T145]` (host.cpp:343) SBT。
`[K3-T146]` (host.cpp:344) raygen（1 record）。
`[K3-T147]` (host.cpp:351) miss（1 record）。
`[K3-T148]` (host.cpp:359) TODO [必做] 步骤 9c：hitgroup（NUM_TRIANGLES 条 record，每三角形一条）。
`[K3-T149]` (host.cpp:380) SBT 构造完成。
`[K3-T150]` (host.cpp:382) optixLaunch。
`[K3-T151]` (host.cpp:395) optixLaunch 完成。
`[K3-T152]` (host.cpp:397) 结果读回 + 写出 PPM。
`[K3-T153]` (host.cpp:403) TODO [进阶] 自定义 intersection（球光源）。
`[K3-T154]` (host.cpp:404) TODO [进阶] Phong 着色（加镜面反射项）。
`[K3-T155]` (host.cpp:405) TODO [进阶] 多个点光源循环累加。
`[K3-T156]` (host.cpp:407) 清理。

---

## 进阶任务

- 实现自定义 intersection（球），使光源本身可被打到（解注释 `__intersection__sphere`）
- 改为 Phong 着色（加镜面反射项）
- 支持多个点光源（循环累加贡献）
- 实现 glossy 反射（随机扰动反射方向）

---

## 验收点

- 编译无错误，`programs.optixir` 生成
- 运行无 OptiX API error
- `output_k3.ppm` 可见：Cornell Box 直接照明（近光源亮，远处暗，地面有阴影）
- Nsight Compute：raygen、any-hit、closest-hit、miss 均有调用记录

---

## 常见问题

**Q: 全图是黑色**
A: 检查 `visibility` 计算。可能是阴影光线 miss 时 `p3` 未被正确设为 2，
   导致 `visibility = 0`。暂时注释阴影逻辑确认基础着色是否正常。

**Q: 阴影覆盖全图（全黑）**
A: `tmax` 设置有误，阴影光线超过光源打到后墙。
   确保 `tmax = light_dist - epsilon`。

**Q: `optixTrace` 崩溃或返回错误**
A: `payloadValues` 与调用中的 payload 槽位数不一致。
   K3 需要 `numPayloadValues = 4` 且 `optixTrace` 传入 `p0..p3`。

---

## 参考资料

- OptiX 8 Guide - Program Types:
  https://raytracing-docs.nvidia.com/optix8/guide/index.html#program_types
- OptiX 8 Guide - Recursion and Depth:
  https://raytracing-docs.nvidia.com/optix8/guide/index.html#recursion

---

## 输出对照（printf / std::puts 原文）

- `[K3-T94]` (host.cpp:22) 原文：`[K3] GPU_STUDY_NO_OPTIX=1: OptiX SDK 未安装。` → 现：`[K3] GPU_STUDY_NO_OPTIX=1: OptiX SDK is not installed.`
- `[K3-T95]` (host.cpp:24) 原文：`     请参考 K1/README.md 完成安装后重新配置 CMake。` → 现：`     See K1/README.md and re-configure CMake after installation.`
- `[K3-T97]` (host.cpp:69) 原文：`无法打开: %s\n` → 现：`Cannot open: %s\n`
- `[K3-T118]` (host.cpp:153) 原文：`  输出已写入: %s\n` → 现：`  Output written: %s\n`
- `[K3-T120]` (host.cpp:160) 原文：`[K3] intersection_anyhit_closesthit_miss` → 现：保持英文不变
- `[K3-T127]` (host.cpp:236) 原文：`  GAS 构建完成` → 现：`  GAS build complete`
- `[K3-T137]` (host.cpp:307) 原文：`  program groups 创建完成` → 现：`  program groups created`
- `[K3-T140]` (host.cpp:321) 原文：`  pipeline 创建完成` → 现：`  pipeline created`
- `[K3-T149]` (host.cpp:380) 原文：`  SBT 构造完成` → 现：`  SBT built`
- `[K3-T151]` (host.cpp:395) 原文：`  optixLaunch 完成` → 现：`  optixLaunch finished`
- `[K3-T157]` (host.cpp:425) 原文：`[K3] 完成。` → 现：`[K3] done.`
