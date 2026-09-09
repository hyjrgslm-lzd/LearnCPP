// ============================================================
// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-3
// C++ 标准要求: C++20
// 本题目标: 手写最小 RDG Compute Pass (implement-own)
//
// 骨架阶段预期行为 (PIE 启动后 Output Log):
//   LogExH3: ExH3 ComputePass 模块已启动, Shader 目录映射已注册
//   LogExH3: [RenderThread] ExH3_AddComputePass 被调用 (骨架: 未实际 AddPass)
//
// 完成后预期行为:
//   RDG 执行时 GPU 录制 ExH3 compute pass, RenderDoc 可捕获
//   Output Log: [RenderThread] ExH3 AddPass 完成, 等待 Execute() 调用 lambda
// ============================================================

#include "ExH3_ComputePass.h"
#include "ShaderParameterUtils.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FExH3ComputePassModule, ExH3_ComputePass);

DEFINE_LOG_CATEGORY_STATIC(LogExH3, Log, All);

// ============================================================
// FExH3ComputeShader 全局 Shader 注册
//
// IMPLEMENT_GLOBAL_SHADER 宏将 FExH3ComputeShader 与 .usf 文件绑定,
// 在静态初始化阶段注册到全局 shader map。
//
// 骨架阶段: .usf 文件尚未创建, 编译时 shader 会 miss，但模块本身可编译。
// 完成阶段: 在 Shaders/ExH3Compute.usf 里实现 MainCS。
//
// TODO [必做] 1: 创建 Shaders/ExH3Compute.usf 文件，内容示例:
//   #include "/Engine/Public/Platform.ush"
//   Texture2D<float4>    InputTexture;
//   RWTexture2D<float4>  OutputTexture;
//   [numthreads(8, 8, 1)]
//   void MainCS(uint3 DTid : SV_DispatchThreadID)
//   {
//       OutputTexture[DTid.xy] = InputTexture.Load(int3(DTid.xy, 0));
//   }
// ============================================================

// 骨架阶段将 IMPLEMENT_GLOBAL_SHADER 注释掉，避免因缺少 .usf 导致编译器报错
// 完成 TODO [必做] 1 后取消注释:
// IMPLEMENT_GLOBAL_SHADER(FExH3ComputeShader, "/ExH3Shaders/ExH3Compute.usf", "MainCS", SF_Compute);

// ============================================================
// ExH3_AddComputePass 实现
// ============================================================

void ExH3_AddComputePass(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FRDGTextureRef OutputTexture,
    const FIntPoint& TextureSize)
{
    // 此函数应在 RenderThread 上调用
    // (例: 在 ISceneViewExtension::PostRenderViewFamily_RenderThread 里)
    check(IsInRenderingThread());

    UE_LOG(LogExH3, Log, TEXT("[RenderThread] ExH3_AddComputePass 被调用 (骨架: AddPass 已注释)"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 取消下列注释块，完成完整的 AddPass 调用
    //
    // 步骤说明:
    //   1. AllocParameters  — 将参数 struct 分配到 RDG 持有的 arena
    //                          不能在栈上分配！lambda 在 Execute() 才调用，届时栈帧已销毁
    //   2. 填充参数          — InputTexture 直接赋值 (SRV)
    //                          OutputTexture 需先 CreateUAV (SHADER_PARAMETER_RDG_TEXTURE_UAV 类型)
    //   3. 获取 shader 实例  — TShaderMapRef<> 从全局 shader map 取，不 new
    //   4. AddPass           — 描述期: lambda 存入 Passes[], 不执行任何 GPU 命令
    //                          ERDGPassFlags::Compute 标记此为 compute pass (非 Raster)
    //   5. lambda 内调用 FComputeShaderUtils::Dispatch — 只在 Execute() 期间调用
    // ══════════════════════════════════════════════════════

    /*
    // Step 1: 分配参数 struct (生命周期绑定到 GraphBuilder)
    FExH3ComputePassParameters* Parameters =
        GraphBuilder.AllocParameters<FExH3ComputePassParameters>();

    // Step 2: 填充参数
    Parameters->InputTexture  = InputTexture;
    Parameters->OutputTexture = GraphBuilder.CreateUAV(OutputTexture);
    // 注意: CreateUAV 要求 OutputTexture 创建时声明了 TexCreate_UAV flag
    // 若未声明, RDG 验证层会报错 (仅 RDG_ENABLE_DEBUG 为真时)

    // Step 3: 获取 shader 实例 (从全局 shader map, 不 new)
    TShaderMapRef<FExH3ComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    // Step 4: AddPass (描述期 — 此时不执行任何 RHI API, 不产生 GPU 命令)
    //
    // 注意: ERDGPassFlags::Compute 与 ERDGPassFlags::AsyncCompute 的区别:
    //   Compute      — 在主 graphics pipeline 顺序执行
    //   AsyncCompute — 在 async compute pipeline 并行执行 (须有显式 fence 处理依赖)
    //
    // FComputeShaderUtils::AddPass 内部等价于:
    //   GraphBuilder.AddPass(EventName, Parameters, ERDGPassFlags::Compute,
    //       [ComputeShader, Parameters, GroupCount](FRHIComputeCommandList& RHICmdList)
    //       {
    //           FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *Parameters, GroupCount);
    //       });
    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("ExH3_ComputePass"),
        ComputeShader,
        Parameters,
        FComputeShaderUtils::GetGroupCount(TextureSize, FIntPoint(8, 8)));
    // AddPass 返回后, lambda 仍未执行
    // 只有调用方调用 GraphBuilder.Execute() 时, lambda 才被调用:
    //   1. Compile(): 分析 Parameters 字段, 推导 InputTexture → SRV 的 barrier 位置
    //   2. 分配 transient 资源 (若 InputTexture/OutputTexture 是 transient 创建的)
    //   3. 调用此 lambda 录制 GPU 命令
    */

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 在参数 struct 里增加 SHADER_PARAMETER_RDG_BUFFER_UAV
    //               让 compute shader 同时写入一个 structured buffer
    //               观察 RDG 为 buffer 插入的 barrier 类型 (ERHIAccess::UAVCompute)
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2: 实现三 pass pipeline (对应 H1 任务 4):
    //   Heightfield pass → Normal map pass → Gamma correction pass
    //   共享同一个 GraphBuilder, 验证中间 texture 可以 transient 分配
    // ══════════════════════════════════════════════════════
}

// ============================================================
// 模块生命周期
// ============================================================

void FExH3ComputePassModule::StartupModule()
{
    UE_LOG(LogExH3, Log, TEXT("ExH3 ComputePass 模块已启动"));

    // TODO [必做] 3: 取消注释，注册 Shader 源码目录映射
    //   AddShaderSourceDirectoryMapping 告知 UE shader 编译器:
    //   虚路径 "/ExH3Shaders" 对应本模块的 Shaders/ 目录
    //
    // FString ShaderDir = FPaths::Combine(
    //     IPluginManager::Get().FindPlugin(TEXT("ExH3_ComputePass"))->GetBaseDir(),
    //     TEXT("Shaders"));
    // AddShaderSourceDirectoryMapping(TEXT("/ExH3Shaders"), ShaderDir);
    //
    // 注意: 若不注册映射, IMPLEMENT_GLOBAL_SHADER 中的虚路径将解析失败,
    //       报错: "Can't map shader source directory"
    UE_LOG(LogExH3, Log, TEXT("  (骨架: Shader 目录映射已注释, 创建 .usf 后取消注释)"));
}

void FExH3ComputePassModule::ShutdownModule()
{
    UE_LOG(LogExH3, Log, TEXT("ExH3 ComputePass 模块已卸载"));
}

// ---- 验证区 (完成 TODO 后搬入对应代码路径) ----
// ensureMsgf(IsInRenderingThread(),
//     TEXT("ExH3_AddComputePass 必须在 RenderThread 上调用"));
// ensureMsgf(InputTexture != nullptr,
//     TEXT("InputTexture 不能为 nullptr (应由先前 pass 创建或 RegisterExternalTexture)"));
// ensureMsgf(OutputTexture != nullptr,
//     TEXT("OutputTexture 不能为 nullptr (应以 TexCreate_UAV flag 创建)"));
