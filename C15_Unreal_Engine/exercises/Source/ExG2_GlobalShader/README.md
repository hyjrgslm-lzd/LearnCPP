> 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-2

## 目标

派生 `FGlobalShader`，完整走通从 C++ 类声明、`BEGIN_SHADER_PARAMETER_STRUCT` 参数定义、`.usf` 编写、`IMPLEMENT_GLOBAL_SHADER` 静态注册，到 GameThread enqueue → RenderThread dispatch compute 的全链路。深刻理解"C++ 侧 `FParameters` 元数据 → 编译器自动注入 HLSL cbuffer/binding"这一 UE 第二处代码生成机制。

## 前置理解

- 已完成 ExG1_RHIResource
- 理解 `IMPLEMENT_GLOBAL_SHADER` 通过静态初始化在 DLL 加载时（`main()` 之前）完成注册，绑定进程生命周期，**不支持热重载**
- 了解 HLSL `[numthreads(X, Y, Z)]` 语义与 `SV_DispatchThreadID`
- 了解 UAV（Unordered Access View）是 compute shader 写目标
- 源码：`Engine/Source/Runtime/RenderCore/Public/GlobalShader.h`（`DECLARE_GLOBAL_SHADER` 第 408 行，`IMPLEMENT_GLOBAL_SHADER` 第 410 行）
- 源码：`Engine/Source/Runtime/RenderCore/Public/ShaderParameterMacros.h`（`BEGIN_SHADER_PARAMETER_STRUCT` 第 1482 行）

## 必做任务

1. 在 `ExG2_GlobalShader.h` 的 `FExG2ComputeShader::FParameters` 中，取消注释 `SHADER_PARAMETER(uint32, ThreadCount)` 和 `SHADER_PARAMETER_UAV(RWBuffer<uint32>, OutputBuffer)` 声明。
2. 在 `ExG2.usf` 中实现 `MainCS`：当 `DispatchThreadId.x < ThreadCount` 时，将 `DispatchThreadId.x * DispatchThreadId.x` 写入 `OutputBuffer[DispatchThreadId.x]`。
3. 在 `ExG2_GlobalShader.cpp` 的 `ENQUEUE_RENDER_COMMAND` lambda 中：获取 shader 实例、创建 64×uint32 的 UAV buffer、填写 `FParameters`、调用 `FComputeShaderUtils::Dispatch(..., FIntVector(1,1,1))`。
4. 在 Editor Console 输入 `r.DumpShaderDebugInfo 1`，重启后打开 `Saved/ShaderDebugInfo/` 找到 `FExG2ComputeShader` 对应的中间文件，观察编译器自动注入的 `cbuffer` 和 `register` 声明。
5. 用 `ShaderCore.h` 里的 `AddShaderSourceDirectoryMapping` 在 `StartupModule` 中映射 `/Project` 到本模块的 `Shaders/` 目录。

## 进阶任务

- 添加 permutation dimension `FEnableSquaredDim : SHADER_PERMUTATION_BOOL("ENABLE_SQUARED")`，在 `.usf` 里用 `#if ENABLE_SQUARED` 切换逻辑，观察 shader compile 数量翻倍。
- 覆写 `ModifyCompilationEnvironment`，用 `OutEnvironment.SetDefine(TEXT("THREAD_GROUP_SIZE"), 64)` 注入宏定义，在 `.usf` 里用 `[numthreads(THREAD_GROUP_SIZE, 1, 1)]`。
- 阅读 `ShaderParameterMacros.h` 第 1482–1500 行，展开 `BEGIN_SHADER_PARAMETER_STRUCT` / `END_SHADER_PARAMETER_STRUCT` 宏，理解通过 `zzGetMembers()` 链式构建 `FShaderParametersMetadata::FMember` 数组的机制。

## 验收点

- [ ] Editor 启动后 Output Log 出现 "Compiling shader FExG2ComputeShader"
- [ ] `TShaderMapRef<FExG2ComputeShader>` 能取到非 null 的 shader 引用
- [ ] `FComputeShaderUtils::Dispatch` 不触发 RHI validation 错误
- [ ] `r.DumpShaderDebugInfo=1` 后能看到自动注入的 cbuffer 声明
- [ ] 能不查文档口述 `BEGIN_SHADER_PARAMETER_STRUCT` 在 C++ 侧做了什么

## 观察点

- `IMPLEMENT_GLOBAL_SHADER` 展开后是进程级静态变量（`ShaderClass::StaticType`），构造函数在 DLL 加载时调用，将 shader 类型注册进 `FGlobalShaderType` 静态链表——与 `IMPLEMENT_MODULE` 的模块注册机制同构，但**无法**通过重新加载 DLL 触发 shader 重编。
- `GetGlobalShaderMap(GMaxRHIFeatureLevel)` 只在 RenderThread 安全调用；GameThread 调用触发 check 失败。
- `SHADER_USE_PARAMETER_STRUCT` 在 shader 构造函数里调用 `BindForLegacyShaderParameters<FParameters>`，把 `FParameters` 元数据绑定到 shader 的 `Bindings` 对象——C++ 元数据与 shader 绑定点的运行期连接点。

## 常见坑

- **`IMPLEMENT_GLOBAL_SHADER` 放在 .h 里**：多个翻译单元都展开静态变量，产生 ODR 违规，链接器报多重定义。必须放在 .cpp。
- **shader 路径映射缺失**：`AddShaderSourceDirectoryMapping` 未调用，shader 编译器找不到 `.usf`，报 "unable to open shader file"。
- **修改 `.usf` 后无需重新 C++ 编译**，但需要 shader 编译器重跑。Editor 里用 `recompileshaders changed` 控制台命令触发。

## 提示

- `FComputeShaderUtils::Dispatch` 封装了 set compute PSO + dispatch 的常见流程，适合练习；生产代码可能直接调用 `RHICmdList.SetComputePipelineState` + `RHICmdList.DispatchComputeShader`。
- 如果 `TShaderMapRef` 取到 nullptr，检查：① `ShouldCompilePermutation` 返回值；② shader 路径映射是否正确；③ Output Log 里搜索 shader 类名是否有编译错误。

## 复盘问题

1. 真正开始执行的时刻？（`IMPLEMENT_GLOBAL_SHADER` 静态注册在 DLL 加载时；shader 实际编译在引擎初始化期；Dispatch 在 RenderThread；GPU 执行在 RHIThread 提交命令后。）
2. 谁负责这对象的生命周期？（`FExG2ComputeShader::StaticType` 进程级静态；`TShaderMapRef` 是全局 shader map 中已编译实例的弱包装；`FBufferRHIRef`/`FUnorderedAccessViewRHIRef` 由 `TRefCountPtr` 管理。）
3. 涉及哪些 Named Thread？（GameThread 投递；RenderThread 执行 lambda；RHIThread 提交 GPU 命令。）
4. 这对 GC 如何可见？（`FGlobalShader` 不是 UObject，不涉及 GC；shader 参数 struct 里的资源是纯 RHI 引用计数对象，与 GC 完全无关。）
5. [本题专属] `BEGIN_SHADER_PARAMETER_STRUCT` 在 C++ 侧构建的 `FShaderParametersMetadata` 与 UHT 生成的 `UCLASS` 反射元数据有哪三点本质相似、哪三点本质差异？

## 对应官方参考

- `Engine/Source/Runtime/RenderCore/Public/GlobalShader.h`（`DECLARE_GLOBAL_SHADER` 第 408 行，`IMPLEMENT_GLOBAL_SHADER` 第 410 行）
- `Engine/Source/Runtime/RenderCore/Public/ShaderParameterMacros.h`（`BEGIN_SHADER_PARAMETER_STRUCT` 第 1482 行，`SHADER_PARAMETER_UAV` 第 1731 行）
- `Engine/Source/Runtime/RenderCore/Public/ShaderCore.h`（`AddShaderSourceDirectoryMapping` 第 1643 行）
- `Engine/Shaders/Private/ComputeGenerateMips.usf`（真实 compute shader 样例）
