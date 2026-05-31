// ============================================================
// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-3
// C++ 标准要求: C++20
// 本题目标: 传统 RHI Draw Pass（手动 barrier / PSO / BeginRenderPass / DrawPrimitive）
//           刻意暴露三大手工负担，为模块 H 的 RDG 铺垫
//
// 三大手工负担（需在观察记录中列表）：
//   1. Barrier 管理：手动指定资源状态转换（Unknown→RTV，RTV→SRV）
//   2. PSO 设置顺序：ApplyCachedRenderTargets 必须先于 SetGraphicsPipelineState
//   3. Resource 生命周期：RenderTargetTexture 必须在 pass 执行期间保持引用计数 > 0
//
// 骨架阶段预期行为: 模块注册，IMPLEMENT_GLOBAL_SHADER 完成静态注册
// 完成后预期行为: Draw pass 无 D3D12 debug layer 红色报错
// ============================================================

#include "ExG3_RHIPass.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformTLS.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ShaderCore.h"    // AddShaderSourceDirectoryMapping
#include "Misc/Paths.h"
#include "ScreenRendering.h"  // GFilterVertexDeclaration

// ── IMPLEMENT_GLOBAL_SHADER 必须在 .cpp（ODR 规则）─────────────
IMPLEMENT_GLOBAL_SHADER(FExG3PassVS, "/Project/ExG3.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FExG3PassPS, "/Project/ExG3.usf", "MainPS", SF_Pixel);

DEFINE_LOG_CATEGORY_STATIC(LogExG3, Log, All);

// 模块级 render target（生命周期与模块绑定）
static FTextureRHIRef GExG3RenderTarget;

class FExG3Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogExG3, Log, TEXT("[GameThread] ExG3_RHIPass 模块启动，注册 shader 路径映射"));

        // Shader 路径映射（与 G2 同，Projects 依赖的原因）
        const FString ShaderDir = FPaths::ProjectDir() / TEXT("Source/ExG3_RHIPass/Shaders");
        AddShaderSourceDirectoryMapping(TEXT("/Project"), ShaderDir);

        // 投递完整 draw pass 到 RenderThread
        ENQUEUE_RENDER_COMMAND(ExG3_DrawPass)(
            [](FRHICommandListImmediate& RHICmdList)
            {
                UE_LOG(LogExG3, Log, TEXT("[RenderThread] ThreadId=%u，开始传统 RHI Draw Pass"),
                    FPlatformTLS::GetCurrentThreadId());

                // ══════════════════════════════════════════════════════
                // TODO [必做] 1: 创建 256x256 RGBA8 render target texture
                //
                //   FRHITextureCreateDesc RTDesc =
                //       FRHITextureCreateDesc::Create2D(TEXT("ExG3_RenderTarget"), 256, 256, PF_R8G8B8A8);
                //   RTDesc.AddFlags(ETextureCreateFlags::RenderTargetable | ETextureCreateFlags::ShaderResource);
                //   RTDesc.SetInitialState(ERHIAccess::RTV);
                //   GExG3RenderTarget = RHICmdList.CreateTexture(RTDesc);
                // ══════════════════════════════════════════════════════
                UE_LOG(LogExG3, Log, TEXT("[RenderThread] TODO: 创建 256x256 RenderTarget"));

                // ════════════════════════════════════════════════════════════
                // 手工负担 1：Barrier — 将 RT 从未知状态转换到 RTV 可写状态
                // 遗漏此步：GPU 验证层（-d3ddebug）报 "resource state transition required"
                // RDG 如何解决：Execute 期分析 pass 读写声明，自动插入 barrier
                // ════════════════════════════════════════════════════════════
                // TODO [必做] 2: 取消注释并补全（需先完成 TODO 1）
                // if (GExG3RenderTarget.IsValid())
                // {
                //     RHICmdList.Transition(FRHITransitionInfo(
                //         GExG3RenderTarget.GetReference(),
                //         ERHIAccess::Unknown,   // 手工负担：必须知道当前状态
                //         ERHIAccess::RTV));     // 目标：Render Target View 可写
                // }

                // ════════════════════════════════════════════════════════════
                // 手工负担 2：PSO 设置顺序约束
                // ApplyCachedRenderTargets 必须在 SetGraphicsPipelineState 之前
                // 遗漏此步：PSO 的 render target format 字段为 unknown，每帧重建 PSO
                // RDG 如何解决：pass 描述期声明 RT 绑定，Execute 期按正确顺序设置
                // ════════════════════════════════════════════════════════════
                // TODO [必做] 3: 设置 RenderPass + PSO（需先完成 TODO 1/2）
                // if (GExG3RenderTarget.IsValid())
                // {
                //     FRHIRenderPassInfo RPInfo(GExG3RenderTarget.GetReference(),
                //         ERenderTargetActions::Clear_Store);
                //     RHICmdList.BeginRenderPass(RPInfo, TEXT("ExG3_DrawPass"));
                //
                //     TShaderMapRef<FExG3PassVS> VS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                //     TShaderMapRef<FExG3PassPS> PS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                //
                //     FGraphicsPipelineStateInitializer PSOInit;
                //     RHICmdList.ApplyCachedRenderTargets(PSOInit);  // ★ 必须最先
                //     PSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
                //     PSOInit.BlendState        = TStaticBlendState<>::GetRHI();
                //     PSOInit.RasterizerState   = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
                //     PSOInit.PrimitiveType     = PT_TriangleList;
                //     PSOInit.BoundShaderState.VertexShaderRHI  = VS.GetVertexShader();
                //     PSOInit.BoundShaderState.PixelShaderRHI   = PS.GetPixelShader();
                //     PSOInit.BoundShaderState.VertexDeclarationRHI =
                //         GFilterVertexDeclaration.VertexDeclarationRHI;  // 引擎内置全屏顶点格式
                //     SetGraphicsPipelineState(RHICmdList, PSOInit, 0);
                //
                //     // 设置 VS 参数
                //     FExG3PassVS::FParameters VSParams;
                //     VSParams.ViewProjection = FMatrix44f::Identity;
                //     SetShaderParameters(RHICmdList, VS, VS.GetVertexShader(), VSParams);
                //
                //     // 发出 draw call（全屏三角形，3 个顶点，1 个 primitive，1 个 instance）
                //     RHICmdList.DrawPrimitive(0, 1, 1);
                //
                //     RHICmdList.EndRenderPass();
                // }

                // ════════════════════════════════════════════════════════════
                // 手工负担 3：Barrier — 将 RT 从 RTV 转换到 SRVGraphics 可读状态
                // 遗漏此步：后续将该 texture 作为 SRV 使用时 GPU 验证层报错
                // RDG 如何解决：transient pool 把资源生命周期与 pass 生命周期绑定
                // ════════════════════════════════════════════════════════════
                // TODO [必做] 4: 取消注释
                // if (GExG3RenderTarget.IsValid())
                // {
                //     RHICmdList.Transition(FRHITransitionInfo(
                //         GExG3RenderTarget.GetReference(),
                //         ERHIAccess::RTV,          // 刚才作为 RenderTarget 写入
                //         ERHIAccess::SRVGraphics)); // 现在转为 ShaderResource 可读
                // }

                UE_LOG(LogExG3, Log, TEXT("[RenderThread] TODO: 完成三步 barrier + PSO + DrawPrimitive"));

                // ──────────────────────────────────────────────────
                // TODO [进阶] 1: 故意省略步骤 TODO 2 中的第一个 barrier，
                //               用 -d3ddebug 启动参数记录验证层报错信息
                // TODO [进阶] 2: 用 stat PipelineStateCache 观察 PSO cache hit/miss 率
                // ──────────────────────────────────────────────────
            }
        );
    }

    virtual void ShutdownModule() override
    {
        FlushRenderingCommands();
        GExG3RenderTarget = nullptr;
        UE_LOG(LogExG3, Log, TEXT("[GameThread] ExG3_RHIPass 模块卸载，RenderTarget 进入删除队列"));
    }
};

IMPLEMENT_MODULE(FExG3Module, ExG3_RHIPass);

// ---- 手工负担总结表（完成后填入观察记录）----
//
// | 手工负担        | 本题具体位置              | 遗漏后果                  | RDG 如何自动化          |
// |----------------|--------------------------|--------------------------|------------------------|
// | Barrier 管理    | TODO 2（Unknown→RTV）     | GPU 验证错误 / 数据竞争    | Execute 期自动插入      |
// |                | TODO 4（RTV→SRV）         |                          |                        |
// | PSO 设置顺序    | TODO 3 ApplyCachedRT 先于 | PSO 不稳定，每帧重建      | pass 描述期声明 RT 绑定 |
// |                | SetGraphicsPipelineState  |                          |                        |
// | Resource 生命周 | GExG3RenderTarget 必须在  | use-after-free           | transient pool 自动管理 |
// | 期              | pass 执行期间有效          |                          |                        |
