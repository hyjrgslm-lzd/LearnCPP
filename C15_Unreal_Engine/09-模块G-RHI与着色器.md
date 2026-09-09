# 09 模块 G：RHI 与着色器

## 模块目标

反向引用 `01-心智模型` 的 §3 线程分线 与 §5 RDG = lazy graph。

本模块的教学定位是"地基"：模块 H 的 RDG 是建在 `FRHICommandList` 之上的惰性图，如果你不先理解命令列表、RHI 资源、PSO 这三个底层概念，模块 H 的"自动 barrier、transient pool、pass culling"就只是背诵名词。G 模块的任务是让你用"手动挡"开完一整段旅程，H 模块再换回"自动挡"，你才能体会自动化究竟省掉了什么。

**阶梯定位**：use + inspect

做完本模块，你至少要能稳定说清楚：

- `FRHICommandListImmediate` 与 `FRHICommandList` 的区别，以及"录制"与"提交"发生在哪两条线程。
- `FBufferRHIRef` / `FTextureRHIRef` 是引用计数句柄，其背后的 `FRHIResource` 引用计数归零时通过删除队列（而非 `delete`）回收。
- `DynamicRHI` 是平台后端（D3D12/Vulkan/Metal）的抽象层，`FRHICommandList` 是平台无关的录制层。
- `BEGIN_SHADER_PARAMETER_STRUCT` / `END_SHADER_PARAMETER_STRUCT` 是 UE 第二处代码生成——C++ 侧宏展开构建反射元数据，HLSL 侧自动生成匹配的 `cbuffer` / `RootParameter` 布局；两侧必须字段顺序对齐，否则运行期绑定静默错误。
- `IMPLEMENT_GLOBAL_SHADER` 通过静态初始化把 shader 类型注册进全局 shader map，这是"进程生命周期注册"——与 UHT 的 `UCLASS` 注册机制同构，但属于 shader 编译器系统，不涉及 GC 也不支持热重载。
- 传统 RHI draw pass 的三大手工负担：手动 barrier 转换、手动管理 transient buffer/texture 生命周期、PSO 与 CommandList 绑定的顺序约束。这三点是模块 H 的 RDG 要解决的"痛"。

---

## 练习 G1：RHI 资源与 CommandList 基础

### 目标

在 RenderThread 中使用 `FRHICommandListImmediate` 创建 vertex buffer 与 texture，理解 RHI 资源的引用计数生命周期，并通过 `DynamicRHI` 接口的注释理解平台抽象层的分层结构。

### 前置理解

- 已完成模块 F 的 F4 练习（`ENQUEUE_RENDER_COMMAND`），熟悉 GameThread → RenderThread 的值捕获语义。
- 理解 `TRefCountPtr<T>`：`AddRef`/`Release` 是原子操作；引用计数归零后不直接 `delete`，而是进入 RHI 的删除队列，在下一帧帧尾由 `FRHICommandListExecutor` 批量回收（见 `RHIResources.h:65-91`）。
- 了解 GPU 资源的创建通常需要在 RenderThread 侧完成，GameThread 不能直接调用 `RHICmdList.CreateBuffer()`。

### 必做任务

1. 在 `StartupModule` 或者 Editor 触发的 lambda 中，使用 `ENQUEUE_RENDER_COMMAND` 把以下工作投递到 RenderThread：

   a. 调用 `FRHIResourceCreateInfo` 准备创建信息（`DebugName`、`BulkData` 或 `nullptr`）。

   b. 使用 `RHICmdList.CreateBuffer(...)` 创建一个简单 vertex buffer，指定 `EBufferUsageFlags::VertexBuffer | EBufferUsageFlags::Static`，大小为 `3 * sizeof(FVector3f)`（三角形三顶点）。将返回值存入 `FBufferRHIRef`。

   c. 使用 `RHICmdList.CreateTexture(...)` 或对应便捷函数创建一个 `64x64` RGBA8 的 2D texture（`ETextureCreateFlags::ShaderResource`）。将返回值存入 `FTextureRHIRef`。

   d. 使用 `RHICmdList.UpdateBufferContents(...)` 向 vertex buffer 写入三角形顶点数据。

   e. 在 RenderThread lambda 末尾打印 `UE_LOG`，记录 `VertexBuffer->GetRefCount()` 和 `Texture->GetRefCount()`。

2. 在 `ShutdownModule` 时（仍在 GameThread），将 `FBufferRHIRef` 和 `FTextureRHIRef` 置为 `nullptr`，观察日志里的析构时机（何时真正回收）。

3. 阅读 `DynamicRHI.h` 开头的 `FDynamicRHI` 类声明，找到 `RHICreateBuffer`（或对等名称）的纯虚函数签名，理解"FRHICommandList 是录制层，FDynamicRHI 才是实际创建 GPU 对象的后端"。

### 进阶任务

- 在 RenderThread lambda 里打印 `GDynamicRHI->GetName()`（如 `"D3D12"` / `"Vulkan"`），观察当前后端名称。
- 尝试在 GameThread 直接调用 `GDynamicRHI->RHICreateBuffer(...)` 绕过 CommandList，观察是否触发 check/assert（提示：通常会报"not on render thread"类断言）。
- 阅读 `FRHIResource::Release`（`RHIResources.h:80-91`），理解"引用计数归零 → `MarkForDelete()` → 批量回收"的流程，与 `TSharedPtr` 的"引用计数归零 → 直接析构"对比。
- 阅读 `ERHIThreadMode`（`RHICommandList.h:99-104`）：`DedicatedThread` vs `Tasks` 两种 RHIThread 模式，理解为什么 RHIThread 是可选的（某些平台禁用后命令在 RenderThread 直接提交）。

### 验收点

- `FBufferRHIRef` 在 RenderThread lambda 内引用计数为 1（只有本地持有）；离开 lambda 后引用计数归 0，日志中能观察到回收。
- `GDynamicRHI->GetName()` 在进阶任务中能成功打印当前 RHI 后端名称。
- 你能用一句话解释"为什么不能在 GameThread 直接 `new FRHIBuffer`"。
- 代码中没有手动 `delete` 任何 `FRHIResource` 子类实例。

### 观察点

- `FRHIResource` 的引用计数 (`AtomicFlags`) 是原子操作，`AddRef`/`Release` 是线程安全的；这与 `TSharedPtr<T, ESPMode::ThreadSafe>` 的引用计数同构，但回收路径完全不同。
- `FBufferRHIRef` 是 `TRefCountPtr<FRHIBuffer>` 的 typedef；`FTextureRHIRef` 是 `TRefCountPtr<FRHITexture>` 的 typedef。当 `TRefCountPtr` 析构时调用 `Release()`，而不是 `delete`，这是 RHI 资源的专属回收约定。
- `FRHICommandListImmediate` 继承自 `FRHICommandList`。`Immediate` 的含义是"当前帧可以立即提交"，但在 `r.RHIThread.Enable=1` 配置下，命令录制仍然在 RenderThread，提交发生在 RHIThread。
- `DynamicRHI` 中每个虚函数对应一个平台实现：`FD3D12DynamicRHI`、`FVulkanDynamicRHI`、`FMetalDynamicRHI` 等；`FRHICommandList` 把平台无关的调用序列化成命令，交由后端执行。

### 常见坑

- 捕获 `FBufferRHIRef` 进 lambda 时必须**按值捕获**，否则 RenderThread 执行时 GameThread 的局部变量栈已销毁（与 F 模块的 `ENQUEUE_RENDER_COMMAND` 捕获原则完全相同）。
- 不要在 GameThread 对 `FRHIResource` 子类调用非 const 方法（如 `Release()`），这会打破 RHI 线程安全约定。
- `FBufferRHIRef` 置 `nullptr` 后资源**不会立即**回收——它进入删除队列，在 RenderThread 处理该帧的末尾才回收。如果你立刻在同一帧再次尝试读取该资源，会导致 use-after-free。

### 提示

- `RHICreateBuffer` 在不同 UE 版本有轻微 API 漂移，UE5.7 推荐通过 `FRHICommandListBase::CreateBuffer(const FRHIBufferCreateDesc&)` 来创建。以本机源码 `RHICommandList.h` 为准，搜索 `CreateBuffer` 找到正确签名。
- 打印 `GetRefCount()` 要在 `UE_LOG` 而非 `printf`，因为 `printf` 不保证在 RenderThread 上线程安全。

### 复盘问题（四固定问题）

- **Q1 执行真正开始时刻**：GPU 资源创建和 buffer 内容写入，发生在 RenderThread 排到 `ENQUEUE_RENDER_COMMAND` lambda 时；GPU 侧实际分配内存发生在 `FDynamicRHI::RHICreateBuffer` 返回时（仍在 CPU 侧，但已是 GPU 资源的 CPU-visible handle）。真正的 GPU 读写则更晚——在 CommandList Submit 阶段。
- **Q2 生命周期拥有者**：`FRHIBuffer` / `FRHITexture` 的生命周期由 `FRHIResource` 的引用计数管理，回收通过删除队列而非 RAII 直接析构。`FBufferRHIRef`（即 `TRefCountPtr<FRHIBuffer>`）是引用计数的 C++ 包装，是唯一合法的持有方式。
- **Q3 涉及哪些 Named Thread**：`ENQUEUE_RENDER_COMMAND` 的 lambda 在 **RenderThread** 上执行；资源创建（`RHICreateBuffer`）在 RenderThread 上发出命令，在 **RHIThread**（若启用）上最终提交到驱动。GameThread 只负责投递命令。
- **Q4 涉及 UObject 时怎么对 GC 可见**：本题全程不涉及 `UObject`；`FRHIBuffer`/`FRHITexture` 是纯 C++ 对象，用 `TRefCountPtr`（非 `TSharedPtr`，专属 RHI 引用计数约定）管理，与 GC 完全分离。

### 对应官方参考

- `Engine/Source/Runtime/RHI/Public/RHIResources.h`：`FRHIResource`（:53）、`AddRef`/`Release`（:72-98）、`FRHIBuffer`、`FRHITexture` 定义。
- `Engine/Source/Runtime/RHI/Public/RHICommandList.h`：`FRHICommandList`、`FRHICommandListImmediate`、`ERHIThreadMode`（:99-104）。
- `Engine/Source/Runtime/RHI/Public/DynamicRHI.h`：`FDynamicRHI` 虚函数表，`FShaderResourceViewInitializer`。
- 官方文档：[Unreal Engine RHI Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/graphics-programming-overview-for-unreal-engine)

---

## 练习 G2：FGlobalShader Compute 走一遍

### 目标

派生 `FGlobalShader`，完整走通从 C++ 类声明、shader parameter struct 定义、`.usf` 编写、`IMPLEMENT_GLOBAL_SHADER` 静态注册，到 GameThread enqueue → RenderThread dispatch compute 的全链路。

**UE 特有点（必须理解）**：这是课程中"代码生成"的第二处——`BEGIN_SHADER_PARAMETER_STRUCT` / `END_SHADER_PARAMETER_STRUCT` 宏在 C++ 侧构建 `FShaderParametersMetadata` 反射表，shader 编译器从这份元数据在 HLSL 侧**自动生成** `cbuffer` 绑定代码和 Root Parameter 布局。C++ 的字段声明顺序决定了 HLSL 的内存布局，两侧必须一致。

**UE 特有点（必须理解）**：`IMPLEMENT_GLOBAL_SHADER` 通过**静态初始化**（文件 scope 的静态变量构造）把 shader 类型注册进全局 shader map。这与 `UCLASS` 的 UHT 注册不同：它在进程启动时、`main()` 之前完成，不依赖 UHT，且**绑定进程生命周期**——禁止热重载预期，每次修改 `.usf` 需触发 shader 重新编译（而非 C++ 重新编译）。

### 前置理解

- 理解 G1 的 `FRHICommandListImmediate` 与 RenderThread 的关系。
- 理解模块 F 的 `ENQUEUE_RENDER_COMMAND`：lambda 必须按值捕获。
- 了解 HLSL 的 `[numthreads(X, Y, Z)]` 语义和 `SV_DispatchThreadID`。
- 了解 UAV（Unordered Access View）是 compute shader 写目标。

### 必做任务

**C++ 侧**

1. 在你的 Module 的 `.h` 文件中声明 compute shader 类：

   ```cpp
   class FMyFirstComputeShader : public FGlobalShader
   {
       DECLARE_GLOBAL_SHADER(FMyFirstComputeShader);
       SHADER_USE_PARAMETER_STRUCT(FMyFirstComputeShader, FGlobalShader);

       BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
           SHADER_PARAMETER(uint32, ThreadCount)
           SHADER_PARAMETER_UAV(RWBuffer<uint32>, OutputBuffer)
       END_SHADER_PARAMETER_STRUCT()

       static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
       {
           return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
       }
   };
   ```

2. 在对应的 `.cpp` 文件中放置（且只能在 `.cpp` 中，不能在 `.h` 中）：

   ```cpp
   IMPLEMENT_GLOBAL_SHADER(FMyFirstComputeShader,
       "/Plugin/UELearn/Private/MyFirstCompute.usf",
       "MainCS",
       SF_Compute);
   ```

3. 在 `.Build.cs` 中添加 `"RenderCore"`, `"RHI"`, `"Renderer"` 到 `PrivateDependencyModuleNames`，并添加 shader 路径映射（`AddShaderSourceDirectoryMapping`）。

**HLSL 侧（`.usf` 文件）**

4. 在 `Shaders/Private/MyFirstCompute.usf` 中编写最小 compute shader：

   ```hlsl
   #include "/Engine/Public/Platform.ush"

   uint ThreadCount;
   RWBuffer<uint> OutputBuffer;

   [numthreads(64, 1, 1)]
   void MainCS(uint3 DispatchThreadId : SV_DispatchThreadID)
   {
       if (DispatchThreadId.x < ThreadCount)
       {
           OutputBuffer[DispatchThreadId.x] = DispatchThreadId.x * DispatchThreadId.x;
       }
   }
   ```

5. 注意观察：`uint ThreadCount` 和 `RWBuffer<uint> OutputBuffer` 在 `.usf` 中是裸声明，**不需要**手写 `cbuffer` 或 `register(u0)` 注解——shader 编译器从 C++ 侧的 `FParameters` 元数据**自动生成**绑定。

**Dispatch 侧（RenderThread）**

6. 在 `ENQUEUE_RENDER_COMMAND` 的 lambda 中：

   ```cpp
   [](FRHICommandListImmediate& RHICmdList)
   {
       // 获取已编译的 shader 实例
       TShaderMapRef<FMyFirstComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

       // 创建输出 buffer（RWBuffer<uint32>, 64 个元素）
       FRHIResourceCreateInfo CreateInfo(TEXT("MyOutputBuffer"));
       FBufferRHIRef OutputBuffer = RHICmdList.CreateBuffer(
           64 * sizeof(uint32),
           EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::ShaderResource,
           sizeof(uint32),
           ERHIAccess::UAVCompute,
           CreateInfo);

       // 创建 UAV 视图
       FUnorderedAccessViewRHIRef OutputUAV = RHICmdList.CreateUnorderedAccessView(
           OutputBuffer, PF_R32_UINT);

       // 填参数 struct
       FMyFirstComputeShader::FParameters Params;
       Params.ThreadCount = 64;
       Params.OutputBuffer = OutputUAV;

       // 设置 compute PSO
       FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Params,
           FIntVector(1, 1, 1)); // 1 group * 64 threads = 64 invocations
   };
   ```

7. 触发 shader 编译（在 Editor 中运行或重新 Build），打开 Output Log 观察 `"Compiling shader FMyFirstComputeShader"` 的日志。

8. 打开 `Saved/ShaderDebugInfo/` 目录（若已配置 `r.DumpShaderDebugInfo=1`），找到中间产物，观察**编译器自动生成的** `cbuffer` 和 `UAVRegister` 绑定代码。

### 进阶任务

- 添加 shader permutation dimension：

  ```cpp
  class FEnableSquaredDim : SHADER_PERMUTATION_BOOL("ENABLE_SQUARED");
  using FPermutationDomain = TShaderPermutationDomain<FEnableSquaredDim>;
  ```

  在 `.usf` 中用 `#if ENABLE_SQUARED` 切换计算逻辑；在 C++ dispatch 侧用 `FPermutationDomain Domain; Domain.Set<FEnableSquaredDim>(true)` 选择 permutation；观察日志中 shader compile 数量翻倍。

- 覆写 `ModifyCompilationEnvironment`，向编译器注入宏定义：

  ```cpp
  static void ModifyCompilationEnvironment(
      const FGlobalShaderPermutationParameters& Params,
      FShaderCompilerEnvironment& OutEnvironment)
  {
      FGlobalShader::ModifyCompilationEnvironment(Params, OutEnvironment);
      OutEnvironment.SetDefine(TEXT("THREAD_GROUP_SIZE"), 64);
  }
  ```

- 阅读 `ShaderParameterMacros.h:1482-1500`，展开 `BEGIN_SHADER_PARAMETER_STRUCT` / `END_SHADER_PARAMETER_STRUCT` 的宏，理解其通过链式 `zzGetMembers()` 构建 `FShaderParametersMetadata::FMember` 数组的机制。

### 必读专节：SHADER_PARAMETER 代码生成详解

这是课程中"UE 特有代码生成"的第二处（第一处是 D 模块的 UHT `.generated.h`）。这里详细对照两侧的展开过程。

#### C++ 侧宏展开

`BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )` 展开后，在 `FParameters` 结构体内部插入一系列编译期构建元数据的成员函数和类型别名（见 `ShaderParameterMacros.h:1482-1500`）。核心是 `zzGetMembers()`，它通过一个反向链表（`zzAppendMemberGetPrev`）在编译期枚举所有字段，产生一个 `TArray<FShaderParametersMetadata::FMember>`。

`SHADER_PARAMETER(uint32, ThreadCount)` 展开为：
1. 在 `FParameters` 结构体中声明 `uint32 ThreadCount;` 成员变量（实际内存布局）。
2. 注册一个 `FMember` 描述项：`{Name="ThreadCount", Type=UBMT_UINT32, MemberOffset=offsetof(FParameters, ThreadCount)}`。

`SHADER_PARAMETER_UAV(RWBuffer<uint32>, OutputBuffer)` 类似，但 `Type=UBMT_UAV`，记录的是资源绑定槽位信息而非数据偏移。

`END_SHADER_PARAMETER_STRUCT()` 结尾关闭结构体，并通过 `static FShaderParametersMetadata GetStructMetadata()` 方法把完整的 `FMember` 列表封装为不可变元数据对象，供 shader 系统在编译期和运行期两用。

#### HLSL 侧自动生成

shader 编译器拿到 `FParameters` 的元数据后，在编译 `.usf` 之前**自动前缀注入**一段 HLSL 声明。对本练习，自动生成物形如：

```hlsl
// 自动生成，学员无需手写
cbuffer Parameters
{
    uint ThreadCount;         // 对应 SHADER_PARAMETER(uint32, ThreadCount)
};
RWBuffer<uint> OutputBuffer : register(u0);  // 对应 SHADER_PARAMETER_UAV
```

这意味着你在 `.usf` 中直接用 `ThreadCount` 和 `OutputBuffer`，**不需要**也**不应该**再次声明它们——它们已经由 C++ 侧的 `FParameters` 声明所驱动的代码生成注入进来。如果在 `.usf` 中重复声明，会产生重定义错误。

#### 布局一致性约束

C++ 结构体的字段顺序（通过 `SHADER_PARAMETER` 宏的注册顺序决定）与 HLSL 的 `cbuffer` 内存布局必须对齐。`FShaderParametersMetadata` 在 shader 编译时验证这一点。如果 C++ 侧用 `float3`（12 字节）后接 `float`（4 字节），而 HLSL 侧的 `cbuffer` alignment 规则导致 padding，会产生运行期参数绑定错误，且通常是**静默的**（数值错误而非 crash）。推荐用 `FVector4f` 而非 `FVector3f` 作为 cbuffer 成员，避免 padding 陷阱。

#### 与 UHT `.generated.h` 的对比

| 维度 | UHT 代码生成（D 模块） | SHADER_PARAMETER 代码生成（G 模块）|
|---|---|---|
| 触发时机 | C++ 编译前，UHT 扫描 `.h` | shader 编译前，shader 编译器扫描 metadata |
| 产物 | `.generated.h` 和 `.gen.cpp` | HLSL 前缀注入（临时，不落盘到你的工程）|
| 作用对象 | UObject 反射、GC 可见性 | GPU shader parameter 绑定布局 |
| 可见性 | 可以打开 `Intermediate/` 看产物 | 需要 `r.DumpShaderDebugInfo=1` 才能看中间文件 |
| 热重载 | UObject 可以热重载 | GlobalShader 不支持热重载（静态注册） |
| 破坏纯 C++ 直觉 | 是：宏展开在编译器前跑 | 是：C++ struct 的内存布局控制 HLSL 绑定表 |

**学员动手任务**：在 Editor Console 输入 `r.DumpShaderDebugInfo 1`，重启 Editor（或触发 shader invalidate），然后打开 `Saved/ShaderDebugInfo/<platform>/` 目录。找到 `FMyFirstComputeShader` 对应的 `.usf` 中间文件，观察编译器在你的 `.usf` 源码前面自动插入了哪些 `cbuffer` 和 `register` 声明。这是理解"C++ 与 HLSL 双侧协作"最直接的手段。

### 验收点

- Editor 中运行后，Output Log 出现 `"Compiling global shader FMyFirstComputeShader"`。
- `TShaderMapRef<FMyFirstComputeShader>` 能成功取到非 null 的 shader 引用。
- `FComputeShaderUtils::Dispatch` 调用不触发 RHI validation 错误。
- 你能不查文档，口述 `BEGIN_SHADER_PARAMETER_STRUCT` 宏在 C++ 侧做了什么。
- 进阶任务中，permutation 计数变为 2，shader compile 日志中出现两条 `FMyFirstComputeShader`。

### 观察点

- `IMPLEMENT_GLOBAL_SHADER` 展开后是一个静态变量定义（`ShaderClass::StaticType`），其构造函数在 `main()` 之前（DLL 加载时）调用，将 shader 类型注册进 `FGlobalShaderType` 的静态链表。这和 `IMPLEMENT_MODULE` 的模块注册机制同构，但 **无法** 通过重新加载 DLL 触发 shader 重编——要修改 `.usf`，需要触发 shader cache 失效。
- `GetGlobalShaderMap(GMaxRHIFeatureLevel)` 只在 RenderThread 安全调用。GameThread 调用会触发 check 失败。
- `SHADER_USE_PARAMETER_STRUCT` 宏（`ShaderParameterStruct.h:62-64`）在 shader 构造函数里调用 `BindForLegacyShaderParameters<FParameters>`，把 `FParameters` 的元数据绑定到 shader 的 `Bindings` 对象——这是 C++ 元数据与 shader 绑定点的运行期连接点。
- `ShouldCompilePermutation` 返回 `false` 时，对应 permutation 不会被编译进 shader cache，`TShaderMapRef` 在运行期取到 `nullptr`，`check(!ComputeShader.IsNull())` 会失败。生产代码须在 dispatch 前检查 `ComputeShader.IsValid()`。

### 常见坑

- `IMPLEMENT_GLOBAL_SHADER` **必须在 `.cpp` 文件中**，不能在 `.h` 中。如果放在 `.h`，多个翻译单元都会展开这个静态变量，产生 ODR 违规（多重定义），链接器报错。
- shader 路径映射必须在 `.Build.cs` 中通过 `PrivateIncludePaths` 或 `AddShaderSourceDirectoryMapping` 告知 UBT，否则 shader 编译器找不到 `.usf` 文件，报"unable to open shader file"。
- 修改 `.usf` 后不需要重新 C++ 编译，但**需要** shader 编译器重跑。在 Editor 中可通过 `recompileshaders changed` 控制台命令触发；修改 C++ 侧的 `FParameters` 结构后，C++ 重新编译即可（shader 元数据随 C++ 编译产物更新）。
- 在非 Editor 构建（Shipping/Development）中，`r.DumpShaderDebugInfo` 默认为 0，中间文件不落盘。本套练习统一 Editor-only，所以默认能看到中间文件。

### 提示

- `FComputeShaderUtils` 工具类（`RenderCore`）封装了 set-PSO + dispatch 的常见流程，适合初学练习；生产代码可能直接调用 `RHICmdList.SetComputePipelineState` + `RHICmdList.DispatchComputeShader`。
- 如果 `TShaderMapRef` 取到 `nullptr`，首先检查：① `ShouldCompilePermutation` 返回值；② shader 路径映射是否正确；③ shader 是否有编译错误（Output Log 中搜索 shader 类名）。
- HLSL 文件以 `.usf` 为扩展名（Unreal Shader File），不是 `.hlsl`。UE 的 shader 编译器先预处理 `#include` 替换，再转发给平台 HLSL 编译器（DXC/FXC/SPIRV-Cross）。

### 复盘问题（四固定问题）

- **Q1 执行真正开始时刻**：`IMPLEMENT_GLOBAL_SHADER` 产生的静态变量构造在**进程启动（DLL 加载）时**完成注册；shader 实际编译发生在引擎初始化阶段（`CompileGlobalShaderMap` 调用期）；Dispatch 发生在 **RenderThread** 排到 `ENQUEUE_RENDER_COMMAND` lambda 时；GPU 执行发生在 **RHIThread**（或 RenderThread，取决于 RHIThread 是否启用）提交命令后。
- **Q2 生命周期拥有者**：`FMyFirstComputeShader::StaticType` 是进程级静态对象，生命周期绑定进程；`TShaderMapRef<FMyFirstComputeShader>` 是对全局 shader map 中已编译实例的弱包装（不持有所有权）；`FBufferRHIRef`/`FUnorderedAccessViewRHIRef` 由 `TRefCountPtr` 引用计数管理。
- **Q3 涉及哪些 Named Thread**：GameThread 投递 `ENQUEUE_RENDER_COMMAND`；RenderThread 执行 lambda（获取 shader ref、填 FParameters、调用 Dispatch）；RHIThread 提交 GPU 命令。
- **Q4 涉及 UObject 时怎么对 GC 可见**：`FGlobalShader` 不是 `UObject`，不涉及 GC；shader 参数 struct 里的资源（`FBufferRHIRef`、`FUnorderedAccessViewRHIRef`）是纯 RHI 引用计数对象，与 GC 完全无关。如果 compute shader dispatch 触发的上下文本身持有 `UObject*`（如计算的数据来源于 `UMeshComponent`），则需在 GameThread 捕获时通过 `TWeakObjectPtr` + 有效性检查来确保 GC 安全。

### 对应官方参考

- `Engine/Source/Runtime/RenderCore/Public/GlobalShader.h`：`DECLARE_GLOBAL_SHADER`、`IMPLEMENT_GLOBAL_SHADER` 宏（:335-410）、`FGlobalShaderType`（:86）。
- `Engine/Source/Runtime/RenderCore/Public/ShaderParameterMacros.h`：`BEGIN_SHADER_PARAMETER_STRUCT`（:1482）、`END_SHADER_PARAMETER_STRUCT`（:1485）。
- `Engine/Source/Runtime/RenderCore/Public/ShaderParameterStruct.h`：`SHADER_USE_PARAMETER_STRUCT`（:62）、`GetShaderParameterResourceRHI`（:89）。
- `Engine/Shaders/Private/ComputeGenerateMips.usf`：真实 compute shader 样例，包含 `[numthreads(8,8,1)]` + `RWTexture2D<float4> MipOutUAV` 的模式（:23-40）。
- 官方文档：[Unreal Shaders Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/shader-development-in-unreal-engine)

---

## 练习 G3：传统 RHI Draw Pass（引出 RDG 动机）

### 目标

用传统 `FRHICommandList` 路径手写一个最小全屏 draw pass（vertex shader + pixel shader + PSO + render target set + draw call），刻意体会"每个资源读写都必须手动管理 barrier、transient 资源生命周期与 CommandList 绑死、PSO 设置顺序约束"的痛点，为理解模块 H 的 RDG 动机建立切身体验。

**指向模块 H**：做完本题，把你手动写的 barrier 调用、资源转换、PSO 设置步骤列成一张表。这张表正是 RDG 在 `Execute()` 阶段自动生成的内容——RDG 的 lazy graph 描述即 sender graph，本地手写清单即 `start(op)` 之后展开的工作。模块 H 的第一道练习会直接引用这张表做对照。

### 前置理解

- 已完成 G1（RHI 资源创建）和 G2（FGlobalShader + parameter struct）。
- 了解 GPU 资源状态转换（resource state / barrier）的基本概念：同一块内存在 GPU 不同阶段有不同的访问模式（render target / shader resource / present），切换时需要插入 barrier 通知 GPU 硬件序列化访问。
- 了解 PSO（Pipeline State Object）的概念：把 vertex shader、pixel shader、rasterizer state、blend state、depth-stencil state 打包成一个不可变对象，切换整套渲染状态的代价远低于逐项设置。
- 了解 `FGraphicsPipelineStateInitializer`：UE 对 PSO 描述信息的封装。

### 必做任务

1. **声明 vertex shader 和 pixel shader**：

   ```cpp
   class FMyPassVS : public FGlobalShader
   {
       DECLARE_GLOBAL_SHADER(FMyPassVS);
       SHADER_USE_PARAMETER_STRUCT(FMyPassVS, FGlobalShader);
       BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
           SHADER_PARAMETER(FMatrix44f, ViewProjection)
       END_SHADER_PARAMETER_STRUCT()
   };

   class FMyPassPS : public FGlobalShader
   {
       DECLARE_GLOBAL_SHADER(FMyPassPS);
       SHADER_USE_PARAMETER_STRUCT(FMyPassPS, FGlobalShader);
       BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
           SHADER_PARAMETER_TEXTURE(Texture2D, InputTexture)
           SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
           RENDER_TARGET_BINDING_SLOTS()
       END_SHADER_PARAMETER_STRUCT()
   };
   ```

   在 `.cpp` 中：

   ```cpp
   IMPLEMENT_GLOBAL_SHADER(FMyPassVS, "/Plugin/UELearn/Private/MyPass.usf", "MainVS", SF_Vertex);
   IMPLEMENT_GLOBAL_SHADER(FMyPassPS, "/Plugin/UELearn/Private/MyPass.usf", "MainPS", SF_Pixel);
   ```

2. **创建 render target**（在 RenderThread lambda 内）：

   创建一个 256x256 RGBA8 texture，`ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource`，作为 draw pass 的输出。

3. **手动 barrier：转换 render target 到可写状态**：

   ```cpp
   // 将 RT texture 从 "未知状态"（或 ShaderResource）转换到 RenderTarget 可写状态
   RHICmdList.Transition(FRHITransitionInfo(
       RenderTargetTexture,
       ERHIAccess::Unknown,      // 当前状态（引擎内部追踪或手动指定）
       ERHIAccess::RTV));        // 目标状态：Render Target View
   ```

   注意：这一行是**手工负担 1**——你必须知道资源当前处于什么状态，目标状态是什么，在正确位置插入。遗漏会导致 GPU 验证层（D3D12 debug layer 或 Vulkan validation layer）报错，或者更糟糕的情况：在 release 模式下产生静默数据竞争。

4. **设置 render pass 和 PSO**：

   ```cpp
   // 开始 render pass（设置 render target 绑定）
   FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::Clear_Store);
   RHICmdList.BeginRenderPass(RPInfo, TEXT("MyPass"));

   // 取 shader 实例
   TShaderMapRef<FMyPassVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
   TShaderMapRef<FMyPassPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

   // 填写 PSO 初始化器（手工负担 2：每个状态项必须手动填写，无法只改一项）
   FGraphicsPipelineStateInitializer PSOInit;
   RHICmdList.ApplyCachedRenderTargets(PSOInit);
   PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
   PSOInit.BlendState = TStaticBlendState<>::GetRHI();
   PSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
   PSOInit.PrimitiveType = PT_TriangleList;
   PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
   PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
   PSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;

   SetGraphicsPipelineState(RHICmdList, PSOInit, 0);
   ```

5. **设置 shader 参数并发出 draw call**：

   ```cpp
   // 设置 VS 参数
   FMyPassVS::FParameters VSParams;
   VSParams.ViewProjection = FMatrix44f::Identity;
   SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), VSParams);

   // 设置 PS 参数
   FMyPassPS::FParameters PSParams;
   PSParams.InputTexture = InputTexture;
   PSParams.InputSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
   SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PSParams);

   // 发出 draw call（全屏三角形，3 个顶点）
   RHICmdList.DrawPrimitive(0, 1, 1);

   RHICmdList.EndRenderPass();
   ```

6. **手动 barrier：转换 render target 到 ShaderResource 可读状态**（手工负担 3）：

   ```cpp
   RHICmdList.Transition(FRHITransitionInfo(
       RenderTargetTexture,
       ERHIAccess::RTV,            // 刚才作为 RenderTarget 写入
       ERHIAccess::SRVGraphics));  // 现在作为 Shader Resource 读取
   ```

7. **把三大手工负担列成表**，填入你的观察记录（见"必做任务 7"的格式提示）。

### 进阶任务

- 在 `SetGraphicsPipelineState` 之前打印当前 `PSOInit` 的 hash（通过 `GetTypeHash(PSOInit)` 或类似接口），用 `stat PipelineStateCache` console 命令观察 PSO cache hit/miss 率——理解为什么 UE 要用 `GetAndOrCreateGraphicsPipelineState` 缓存 PSO 而不是每帧重建。
- 故意**省略**步骤 3（barrier：Unknown → RTV）后跑一次，记录 D3D12 debug layer 或 Vulkan validation layer 的报错信息——理解"barrier 遗漏是静默错误还是立即崩溃"在不同平台上的表现差异。
- 阅读 `PipelineCacheUtilities.h` 中 `FPipelineCacheFileFormatPSO` 结构，理解 UE 的 PSO 预热（pre-warm）机制：PSO 创建是 CPU 密集操作，可以提前离线编译并缓存，避免首帧卡顿。
- 对比 D3D12 时代（2015 年前后）的"裸 `ID3D12GraphicsCommandList::ResourceBarrier`"调用：那时候每个 barrier 是手写一个 `D3D12_RESOURCE_TRANSITION_BARRIER` 结构，填 `before state`、`after state`、resource pointer；现在 UE 的 `FRHICommandList::Transition` 是平台无关的包装，但语义是等价的。

### 把手工负担列成表（"痛"的形式化）

做完本练习，把以下三项手工负担用你自己的话填进表里，这张表将在模块 H 中直接复用：

| 手工负担 | 在本练习中的具体位置 | 遗漏后果 | RDG 如何自动化 |
|---|---|---|---|
| **Barrier 管理** | 步骤 3（Unknown→RTV）、步骤 6（RTV→SRV） | GPU 验证错误 / 数据竞争 | RDG 在 Execute 期分析 pass 的读写声明，自动插入 barrier |
| **PSO 设置顺序** | 步骤 4 中 `ApplyCachedRenderTargets` 必须在 `SetGraphicsPipelineState` 之前 | PSO 状态不一致，可能裂图或 crash | RDG pass 描述期声明 RT 绑定，Execute 期自动按顺序设置 |
| **Resource 生命周期** | RenderTargetTexture 必须在 pass 执行期间保持有效，不能在 lambda 结束前 release | use-after-free | RDG transient pool 把资源生命周期与 pass 生命周期绑定，pass 结束自动回收 |

> 这张表就是"模块 H 的第一道练习为什么要先回顾 G3"的原因。RDG 的 lazy graph 把这三列"手工负担"全部变成了由 Execute 阶段统一处理的自动行为——正如 stdexec 的 `start(op)` 把所有 sender 描述的工作在一个统一时刻展开执行。

### 验收点

- Draw pass 能无验证错误地运行（D3D12 debug layer 开启时无红色报错）。
- 三大手工负担表已填完，内容具体而非模板复制粘贴。
- 你能用一句话解释"为什么 PSO 不能每帧重建"（提示：PSO 编译是驱动层的 shader 链接操作，开销巨大）。
- 进阶任务中故意省略 barrier 后，能记录到具体的验证层报错信息。

### 观察点

- `TStaticDepthStencilState<false, CF_Always>::GetRHI()`、`TStaticBlendState<>::GetRHI()`、`TStaticRasterizerState<FM_Solid, CM_None>::GetRHI()` 是 UE 的静态 RHI 状态缓存（见 `RHIStaticStates.h`）——通过模板参数在编译期区分状态，运行期只创建一次并缓存。这是 UE 避免每帧重建渲染状态对象的另一个层次的优化，与 PSO cache 互补。
- `FGraphicsPipelineStateInitializer` 是值语义：你把 VS/PS/状态描述填进去，调用 `SetGraphicsPipelineState`，内部通过 hash 查 cache；如果 cache miss，驱动层编译 PSO（开销大，应避免在热路径上发生）；如果 cache hit，直接绑定已有 PSO（开销小）。
- `BeginRenderPass`/`EndRenderPass` 必须配对。两者之间的命令假设 render target 处于 RTV 状态；结束后 render target 状态变为 resolve/store，需要额外 barrier 才能作为 SRV 使用。
- 传统 pass 里，资源（RenderTargetTexture、InputTexture）的有效性完全由你维护：只要 CommandList 在 RHIThread 上执行时引用计数 > 0，资源就有效。如果你在提交前 release 了资源，RHI 层通常只能在验证层或 crash 时发现问题。

### 常见坑

- **`ApplyCachedRenderTargets` 必须先于填写其他 PSO 字段**（或至少在 `SetGraphicsPipelineState` 之前），否则 PSO 的 render target format 字段为 unknown，导致 PSO hash 不稳定，每帧重建 PSO。
- **Barrier 状态跟踪**：UE 的 RHI 层在 Debug 模式下（`-d3ddebug` 或 `-vulkandebug` 启动参数）会验证你指定的"当前状态"是否与实际状态匹配；Release 模式下不验证，错误的状态说明会被忽略但 GPU 行为未定义。
- **Shader parameter 设置必须在 `SetGraphicsPipelineState` 之后**，因为 PSO 绑定决定了 root signature（D3D12）或 pipeline layout（Vulkan），参数绑定槽位需要在 PSO 已知的情况下才能正确映射。

### 提示

- 全屏三角形的顶点数据通常通过 vertex shader 内嵌顶点坐标（`SV_VertexID` 方案）而不需要 vertex buffer，这样能绕开 G1 的 vertex buffer 创建复杂度，专注于 pipeline state 和 barrier 练习。
- `GFilterVertexDeclaration` 是引擎内置的全屏四边形顶点格式，可以直接引用（需 `RenderCore` 依赖）。
- 如果完整实现 draw pass 难度过大，可以先只写 barrier 和 PSO 设置部分，在 `DrawPrimitive` 之前加 `return`，确认前半段无验证错误后再补全 draw call。

### 复盘问题（四固定问题）

- **Q1 执行真正开始时刻**：`RHICmdList.BeginRenderPass` 是命令录制（RenderThread）；实际 GPU 执行（barrier 生效、rasterization 开始）发生在 **RHIThread** 提交 CommandList 给驱动之后。两者之间存在一帧左右的延迟（RenderThread 通常落后 GameThread 0-1 帧，RHIThread 追随 RenderThread）。
- **Q2 生命周期拥有者**：RenderTargetTexture（`FTextureRHIRef`）由 `TRefCountPtr` 管理；PSO 对象（`FGraphicsPipelineState`）由 `PipelineStateCache` 的 hash map 持有；`FGraphicsPipelineStateInitializer` 是栈上值，不持有任何资源所有权。
- **Q3 涉及哪些 Named Thread**：GameThread 投递命令；**RenderThread** 录制（`BeginRenderPass`、`Transition`、`SetGraphicsPipelineState`、`DrawPrimitive`、`EndRenderPass`）；**RHIThread** 提交并执行（资源状态转换、PSO bind、实际 draw call 到 GPU 驱动）。三条线程全部参与，这是模块 G 中唯一覆盖三条 Named Thread 的练习。
- **Q4 涉及 UObject 时怎么对 GC 可见**：传统 RHI draw pass 全程不涉及 `UObject`；输入 texture（`FTextureRHIRef`）是 RHI 对象，render target（`FTextureRHIRef`）同；如果 draw pass 的数据来源于场景的 `UStaticMeshComponent`，需在 GameThread 提取数据时通过 `UPROPERTY` / `TObjectPtr` 确保 GC 可见，然后按值捕获进入 lambda。

### 对应官方参考

- `Engine/Source/Runtime/RHI/Public/RHICommandList.h`：`BeginRenderPass`、`Transition`、`DrawPrimitive`、`SetGraphicsPipelineState`。
- `Engine/Source/Runtime/RHI/Public/RHIStaticStates.h`：`TStaticDepthStencilState`、`TStaticBlendState`、`TStaticRasterizerState`（均通过模板参数静态缓存）。
- `Engine/Source/Runtime/RenderCore/Public/PipelineCacheUtilities.h`：`FPipelineCacheFileFormatPSO`（PSO 预热缓存格式）。
- UE 官方博客：[Render Dependency Graph](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine)（G3 结束后立即阅读，感受 RDG 动机）。

---

## 做完本模块后你现在应该能说清楚什么

1. **RHI 分层**：`FRHICommandList`（录制，RenderThread）→ `FDynamicRHI`（平台后端，RHIThread）→ GPU 驱动（D3D12/Vulkan/Metal）。三层各自的职责和执行线程。

2. **RHI 资源生命周期**：`FRHIBuffer`/`FRHITexture` 由 `TRefCountPtr`（`FBufferRHIRef`/`FTextureRHIRef`）持有引用计数；归零后进删除队列；不能用 `TSharedPtr` 持有 `FRHIResource` 子类，也不能手动 `delete`。

3. **SHADER_PARAMETER 代码生成**：`BEGIN_SHADER_PARAMETER_STRUCT` 在 C++ 侧建立 `FShaderParametersMetadata` 反射表；shader 编译器用这份元数据在 HLSL 侧自动注入 `cbuffer` 和 resource binding 声明；两侧字段顺序和类型必须对齐；这是 UE 第二处代码生成，与 UHT `.generated.h` 性质相同但作用域不同。

4. **GlobalShader 静态注册**：`IMPLEMENT_GLOBAL_SHADER` 产生进程级静态变量，在 DLL 加载时（`main()` 之前）完成注册；绑定进程生命周期；不支持热重载；shader 修改通过 shader cache invalidation 路径重编，与 C++ 重编分离。

5. **传统 draw pass 三大手工负担**：barrier 手动管理、PSO 设置顺序约束、transient 资源生命周期手工维护——这三点是模块 H 的 RDG 要自动化的核心问题。理解"手动挡的痛"是理解"为什么需要 lazy graph"的前提。

6. **线程分工**：GameThread 只投递命令（`ENQUEUE_RENDER_COMMAND`）；RenderThread 录制 RHI 命令序列；RHIThread 提交给 GPU 驱动。`FRHICommandListImmediate` 是 RenderThread 侧的录制入口，不是"立即执行"的同步接口。

---

## 本模块覆盖的 UE 源码清单

| 文件路径 | 本模块中引用的关键内容 |
|---|---|
| `Engine/Source/Runtime/RHI/Public/RHIResources.h` | `FRHIResource`（:53）、`AddRef`/`Release`（:72-98）、`FRHIBuffer`、`FRHITexture`、`TRefCountPtr` 语义 |
| `Engine/Source/Runtime/RHI/Public/RHICommandList.h` | `FRHICommandList`、`FRHICommandListImmediate`、`ERHIThreadMode`（:99-104）、`BeginRenderPass`/`EndRenderPass`、`Transition`、`DrawPrimitive` |
| `Engine/Source/Runtime/RHI/Public/DynamicRHI.h` | `FDynamicRHI`（平台后端抽象基类）、`FShaderResourceViewInitializer`（:71）|
| `Engine/Source/Runtime/RenderCore/Public/GlobalShader.h` | `FGlobalShaderType`（:86）、`DECLARE_GLOBAL_SHADER`（:408）、`IMPLEMENT_GLOBAL_SHADER`（:410）、静态注册注释（:385-406）|
| `Engine/Source/Runtime/RenderCore/Public/ShaderParameterMacros.h` | `BEGIN_SHADER_PARAMETER_STRUCT`（:1482）、`END_SHADER_PARAMETER_STRUCT`（:1485）、`IMPLEMENT_UNIFORM_BUFFER_STRUCT`（:1566）|
| `Engine/Source/Runtime/RenderCore/Public/ShaderParameterStruct.h` | `SHADER_USE_PARAMETER_STRUCT`（:62）、`SHADER_USE_PARAMETER_STRUCT_INTERNAL`（:50）、`GetShaderParameterResourceRHI`（:89）|
| `Engine/Source/Runtime/RenderCore/Public/ShaderParameters.h` | `FShaderParameter`（:55）、`EShaderParameterFlags`（:46）、`UE::ShaderParameters::CreateUniformBufferShaderDeclaration`（:41）|
| `Engine/Source/Runtime/RenderCore/Public/Shader.h` | `FShader` 基类、`TShaderRef`、`TShaderMapRef`、shader type registration 框架 |
| `Engine/Shaders/Private/ComputeGenerateMips.usf` | `[numthreads(8,8,1)]` compute shader 最小样例（:23-40）、`RWTexture2D<float4> MipOutUAV` UAV 写入模式 |
