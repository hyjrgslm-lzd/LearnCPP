> 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-3

## 目标

从零写出一个最小 RDG Compute Pass：输入一个 `FRDGTextureRef`，输出另一个 `FRDGTextureRef`，让 RDG 自动推导 transition barrier，零行手写 barrier 代码。这是本课程阶梯 implement-own 在渲染层的最终落点。

## 前置理解

- 已完成模块 G（`FGlobalShader` + `BEGIN_SHADER_PARAMETER_STRUCT`）
- 已完成 H1（RDG 三阶段和 stdexec 六维类比表）
- 能说出"`AddPass` 只是在登记工作，`Execute()` 才启动整图"

## 必做任务

1. 创建 `Shaders/ExH3Compute.usf`，实现 `MainCS`（8×8 线程组，复制 InputTexture 到 OutputTexture）
2. 取消 `IMPLEMENT_GLOBAL_SHADER` 宏注释，注册 `FExH3ComputeShader`
3. 在 `StartupModule()` 中注册 Shader 源码目录映射（`AddShaderSourceDirectoryMapping`）
4. 取消 `ExH3_AddComputePass` 函数体内的注释块，完成完整的 `AllocParameters → 填充 → GetShader → AddPass` 流程
5. 在某个 `ISceneViewExtension::PostRenderViewFamily_RenderThread` 里调用 `ExH3_AddComputePass`，确认 pass 出现在 RenderDoc / GPU Profiler 里

## 进阶任务

- 为参数 struct 增加 `SHADER_PARAMETER_RDG_BUFFER_UAV`，让 shader 同时写入 structured buffer
- 实现三 pass pipeline（对应 H1 任务 4）：Heightfield → Normal → Gamma，串联三次 `AddPass`，共享同一 `FRDGBuilder`
- 在笔记中完成传统 `FRHICommandList` vs RDG 行数对比表（barrier 代码 > 15 行 vs 0 行）

## 验收点

- [ ] 代码骨架中 `AddPass` 的 lambda 内没有任何 `RHICmdList.Transition(...)` 调用
- [ ] 能解释 `AllocParameters<>()` 分配的对象为何绑定到 `GraphBuilder` 而不是调用栈帧
- [ ] 能说出为何 `CreateTexture()` 不立即分配 GPU 内存而 `RHICreateTexture2D()` 立即分配
- [ ] 传统 vs RDG 行数对比表数字合理（barrier 代码 > 15 行 vs 0 行）
- [ ] 能脱稿复述 RDG 描述期 vs 执行期（不借助任何资料，5 句话以内）

## 观察点

- `BEGIN_SHADER_PARAMETER_STRUCT` 是 UE 第二处代码生成（第一处是 UHT 的 `.generated.h`），依赖 C++ 宏展开而非 UHT，让 RDG 在运行时能反射参数 struct 的字段
- `ERDGPassFlags::Compute` vs `ERDGPassFlags::AsyncCompute`：前者顺序执行，后者并行；选错不会编译报错但会导致 GPU 时序错误
- `NeverCull` flag：若 pass 输出未被后续 pass 或 `QueueTextureExtraction` 引用，RDG 默认 cull 掉；强制执行 UAV 写入需添加 `ERDGPassFlags::NeverCull`

## 常见坑

- **`Parameters->OutputTexture = OutputTexture`（直接赋 FRDGTextureRef）**：`SHADER_PARAMETER_RDG_TEXTURE_UAV` 字段类型是 `FRDGTextureUAVRef`，必须先 `GraphBuilder.CreateUAV(OutputTexture)`
- **在 lambda 外部调用 `GetRHI()`**：AddPass 时 RHI 资源未分配，只有 Execute() 期间的 lambda 内才可访问
- **`AllocParameters` 在栈上分配**：lambda 延迟执行时调用栈已析构；必须用 `GraphBuilder.AllocParameters<>()`
- **CreateTexture 时忘记 `TexCreate_UAV`**：后续 `CreateUAV` 在 RDG 验证层报错

## 提示

- 搜索 `DeferredShadingRenderer.cpp` 里 `FComputeShaderUtils::AddPass` 的调用示例
- 使用 RenderDoc 捕获帧后在 Event Browser 里搜索 `ExH3_ComputePass` 验证 pass 已执行

## 复盘问题（四固定问题 + 本题专属）

1. **执行真正开始时刻？** `AddPass` 的 lambda 在 `FRDGBuilder::Execute()` 的 pass 遍历阶段被调用，`Compile()` 完成后按拓扑序逐 pass 录制 GPU 命令
2. **生命周期拥有者？** `AllocParameters<>()` 分配的参数 struct 由 `FRDGBuilder` 拥有；transient GPU 显存由 `GRDGTransientResourceAllocator` 拥有，last-reader pass 执行后立即归还
3. **涉及哪些 Named Thread？** `AddPass` 和 `Execute()` 均在 **RenderThread**；GPU 命令由 **RHIThread** 提交驱动
4. **涉及 UObject 时怎么对 GC 可见？** Compute pass 层不涉及 UObject；若需要来自 GameThread 的 `UTexture2D`，通过 `UTexture2D::Resource->TextureRHI` 以 `RegisterExternalTexture` 注入 RDG
5. **[本题专属]** `SHADER_PARAMETER_RDG_TEXTURE` 和 `SHADER_PARAMETER_RDG_TEXTURE_UAV` 宏在 RDG 依赖分析中各代表什么角色？答：前者声明 SRV 读绑定（本 pass 是该资源的 consumer），后者声明 UAV 写绑定（本 pass 是该资源的 producer）；RDG 据此自动推导 barrier 插入位置

## 对应官方参考

- `Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h`（第 200–230 行 `AddPass` 注释）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphUtils.h`（`FComputeShaderUtils::AddPass`，`GetGroupCount`）
- `Engine/Source/Runtime/RenderCore/Public/RenderGraphResources.h`（`FRDGTextureDesc::Create2D`）
- `Engine/Source/Runtime/RenderCore/Public/GlobalShader.h`（`FGlobalShader`，`DECLARE_GLOBAL_SHADER`，`SHADER_USE_PARAMETER_STRUCT`）
- `Engine/Source/Runtime/RenderCore/Public/ShaderParameterMacros.h`（`BEGIN_SHADER_PARAMETER_STRUCT`，`SHADER_PARAMETER_RDG_*`）
