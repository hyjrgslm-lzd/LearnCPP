// ============================================================
// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-1
// C++ 标准要求: C++20
// 本题目标: 在 RenderThread 使用 FRHICommandListImmediate 创建 vertex buffer 与 texture，
//           理解 RHI 资源引用计数生命周期，观察 FDynamicRHI 平台抽象层分层结构
//
// 骨架阶段预期行为: 模块注册，StartupModule 投递 ENQUEUE_RENDER_COMMAND
// 完成后预期行为（Output Log）:
//   LogExG1: [RenderThread] VertexBuffer 引用计数 = 1
//   LogExG1: [RenderThread] Texture 引用计数 = 1
//   LogExG1: [RenderThread] GDynamicRHI 后端名称 = D3D12（或 Vulkan）
//   LogExG1: [GameThread] ShutdownModule 将 FBufferRHIRef 置 nullptr（进入删除队列）
// ============================================================

#include "ExG1_RHIResource.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformTLS.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "DynamicRHI.h"

DEFINE_LOG_CATEGORY_STATIC(LogExG1, Log, All);

// 模块级 RHI 资源句柄（TRefCountPtr 引用计数，生命周期与模块绑定）
// 注意：FBufferRHIRef / FTextureRHIRef 只能在 RenderThread 创建，
//       但可以在 GameThread 将其置为 nullptr（触发引用计数减少，进入删除队列）
static FBufferRHIRef  GExG1VertexBuffer;
static FTextureRHIRef GExG1Texture;

class FExG1Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogExG1, Log, TEXT("[GameThread] ThreadId=%u，投递 RHI 资源创建命令到 RenderThread"),
            FPlatformTLS::GetCurrentThreadId());

        // 所有 RHI 资源创建必须在 RenderThread 侧完成
        // GameThread 只负责投递命令（值捕获语义，F4 已讲）
        ENQUEUE_RENDER_COMMAND(ExG1_CreateResources)(
            [](FRHICommandListImmediate& RHICmdList)
            {
                UE_LOG(LogExG1, Log, TEXT("[RenderThread] ThreadId=%u，开始创建 RHI 资源"),
                    FPlatformTLS::GetCurrentThreadId());

                // ══════════════════════════════════════════════════════
                // TODO [必做] 1: 用 FRHIBufferCreateDesc 创建 vertex buffer
                //
                // UE 5.6+ 现代 API（FRHIResourceCreateInfo 已 deprecated）：
                //   FRHIBufferCreateDesc Desc = FRHIBufferCreateDesc::CreateVertex(
                //       TEXT("ExG1_VertexBuffer"),
                //       3 * sizeof(FVector3f));   // 三角形 3 顶点
                //   Desc.SetInitialState(ERHIAccess::VertexOrIndexBuffer);
                //   GExG1VertexBuffer = RHICmdList.CreateBuffer(Desc);
                //
                // 参考: Engine/Source/Runtime/RHI/Public/RHIResources.h 第 1416 行（FRHIBufferCreateDesc）
                //       Engine/Source/Runtime/RHI/Public/RHICommandList.h 第 800 行（CreateBuffer）
                // ══════════════════════════════════════════════════════

                // 骨架：仅打印占位
                UE_LOG(LogExG1, Log, TEXT("[RenderThread] TODO: 创建 VertexBuffer（3 * sizeof(FVector3f)）"));

                // ══════════════════════════════════════════════════════
                // TODO [必做] 2: 用 FRHITextureCreateDesc 创建 64x64 RGBA8 2D Texture
                //
                //   FRHITextureCreateDesc TexDesc =
                //       FRHITextureCreateDesc::Create2D(TEXT("ExG1_Texture"), 64, 64, PF_R8G8B8A8);
                //   TexDesc.AddFlags(ETextureCreateFlags::ShaderResource);
                //   GExG1Texture = RHICmdList.CreateTexture(TexDesc);
                //
                // 参考: Engine/Source/Runtime/RHI/Public/RHIResources.h（FRHITextureCreateDesc）
                // ══════════════════════════════════════════════════════

                UE_LOG(LogExG1, Log, TEXT("[RenderThread] TODO: 创建 64x64 RGBA8 Texture"));

                // ══════════════════════════════════════════════════════
                // TODO [进阶] 1: 打印 GDynamicRHI->GetName() 查看当前 RHI 后端（D3D12/Vulkan）
                // TODO [进阶] 2: 观察 VertexBuffer->GetRefCount()
                //               离开 lambda 后引用计数应降到 0（触发删除队列）
                // ══════════════════════════════════════════════════════
            }
        );

        UE_LOG(LogExG1, Log, TEXT("[GameThread] RHI 资源创建命令已投递，等待 RenderThread 执行"));
    }

    virtual void ShutdownModule() override
    {
        // 合法的 FlushRenderingCommands 场景：模块卸载（不在 Tick 里）
        FlushRenderingCommands();

        // 将 TRefCountPtr 置为 nullptr：触发引用计数减少
        // 注意：资源不会立即回收，进入 RHI 删除队列，在下一帧帧尾由 FRHICommandListExecutor 批量回收
        GExG1VertexBuffer = nullptr;
        GExG1Texture      = nullptr;

        UE_LOG(LogExG1, Log, TEXT("[GameThread] ExG1_RHIResource 模块卸载，RHI 资源已进入删除队列"));
    }
};

IMPLEMENT_MODULE(FExG1Module, ExG1_RHIResource);

// ---- 验证区 ----
// 完成 TODO 后在 RenderThread lambda 末尾添加：
// ensureMsgf(GExG1VertexBuffer.IsValid(), TEXT("VertexBuffer 应已创建"));
// ensureMsgf(GExG1Texture.IsValid(),      TEXT("Texture 应已创建"));
// ensureMsgf(GExG1VertexBuffer->GetRefCount() == 1, TEXT("引用计数应为 1（只有全局变量持有）"));
