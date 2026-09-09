// ============================================================
// 对应章节: ../../../13-结课项目1-集成项目.md
// C++ 标准要求: C++20
// 本文件: AHeightFieldViewer 实现 — 集成 E/F/G/H 四个模块
//
// 骨架阶段预期行为: Module 注册, Actor 可 Spawn, BeginPlay/EndPlay 打印日志
// 完成后预期行为 (PIE + Console 命令 UELearn.Cap1.Run 256 128):
//   [Cap1][Step 1]  GameThread: UHeightField asset created
//   [Cap1][Step 2]  GameThread: FStreamableManager async load requested
//   [Cap1][Step 3]  GameThread: Load callback received, dispatching UE::Tasks worker
//   [Cap1][Step 4]  Worker: CPU preprocess started (normalize + normal estimate)
//   [Cap1][Step 5]  Worker: CPU preprocess done, enqueuing render command
//   [Cap1][Step 6]  RenderThread: RHI texture upload started
//   [Cap1][Step 7]  RenderThread: RHI texture upload done
//   [Cap1][Step 8]  RenderThread: FRDGBuilder pass 1/2/3 added
//   [Cap1][Step 9]  RenderThread: FRDGBuilder::Execute() called
//   [Cap1][Step 10] GameThread: PNG dump triggered
//
// 三次线程跨越 (约束 §3.1):
//   [Thread Crossing 1] GameThread → UE::Tasks worker (UE::Tasks::Launch)
//   [Thread Crossing 2] UE::Tasks worker → GameThread (AsyncTask GameThread)
//   [Thread Crossing 3] GameThread → RenderThread (ENQUEUE_RENDER_COMMAND)
//
// 约束检查表 (§3.2/§3.3/§3.4):
//   ✓ ≥1 UPROPERTY 包装 UObject: TObjectPtr<UHeightFieldAsset> HeightFieldAsset
//   ✓ ≥1 TSharedPtr 管理非 UObject: TSharedPtr<FHeightFieldPreprocessResult>
//   ✓ ≥1 AActor 承载可视化: AHeightFieldViewer 本身
//   ✓ ≥1 UWorldSubsystem: UHeightFieldWorldSubsystem (HeightFieldSubsystem.h)
//   ✓ 禁 FlushRenderingCommands: 全程异步回调, 无阻塞等待
//   ✓ 禁裸 new UObject: 全用 NewObject<T>
//   ✓ 禁裸 UObject* 成员: 用 TObjectPtr + UPROPERTY
// ============================================================

#include "AHeightFieldViewer.h"
#include "UHeightFieldAsset.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
// 模块 F 知识点: UE::Tasks 投递到 TaskGraph worker pool
// 参考: Engine/Source/Runtime/Core/Public/Tasks/Task.h
#include "Tasks/Task.h"
// 模块 F 知识点: ENQUEUE_RENDER_COMMAND 投递到 RenderThread
// 参考: Engine/Source/Runtime/RenderCore/Public/RenderingThread.h
#include "RenderingThread.h"
// 模块 G 知识点: RHI 纹理创建与上传
// 参考: Engine/Source/Runtime/RHI/Public/RHICommandList.h
#include "RHICommandList.h"
// 模块 H 知识点: FRDGBuilder + pass 宏
// 参考: Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h
#include "RenderGraphBuilder.h"
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"
// 异步线程切换
#include "Async/Async.h"

DEFINE_LOG_CATEGORY_STATIC(LogCap1Viewer, Log, All);

// ══════════════════════════════════════════════════════════════
// 模块 C 知识点: TSharedPtr 管理非 UObject 中间结果
// 约束 §3.2: 禁止用 UCLASS / NewObject 创建此结构
// 约束 §3.2: 使用 ESPMode::ThreadSafe (跨线程引用计数)
// ══════════════════════════════════════════════════════════════
struct FHeightFieldPreprocessResult
{
    // 归一化后的 float 高度数组 [0.0, 1.0]
    TArray<float> NormalizedHeights;

    // 估算法线 (XYZ, 每像素 3 float)
    TArray<FVector3f> EstimatedNormals;

    int32 Resolution = 0;
};

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
AHeightFieldViewer::AHeightFieldViewer()
{
    PrimaryActorTick.bCanEverTick = false;
    HeightFieldAsset = nullptr;
}

// ------------------------------------------------------------
// BeginPlay: 模块 K 钩子 1
// 若 HeightFieldAssetRef 已在 Editor 里指定, 立即触发加载
// ------------------------------------------------------------
void AHeightFieldViewer::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogCap1Viewer, Log, TEXT("[Cap1] AHeightFieldViewer::BeginPlay"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1 (模块 E): 若软引用已设置, 触发异步加载
    // ══════════════════════════════════════════════════════
    if (!HeightFieldAssetRef.IsNull())
    {
        StartAsyncLoad(HeightFieldAssetRef.ToSoftObjectPath());
    }
}

// ------------------------------------------------------------
// EndPlay: 模块 K 钩子 2 — 注销回调, 清理资源
// ------------------------------------------------------------
void AHeightFieldViewer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogCap1Viewer, Log, TEXT("[Cap1] AHeightFieldViewer::EndPlay"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 释放 FStreamableHandle
    // LoadHandle.Reset() 让 StreamableManager 知道我们不再需要该 Asset
    // ══════════════════════════════════════════════════════
    if (LoadHandle.IsValid())
    {
        LoadHandle->ReleaseHandle();
        LoadHandle.Reset();
    }

    HeightFieldAsset = nullptr;

    Super::EndPlay(EndPlayReason);
}

// ------------------------------------------------------------
// StartAsyncLoad: 触发 FStreamableManager 异步加载
// 参考: Engine/Source/Runtime/Engine/Classes/Engine/StreamableManager.h
// 坑6: 必须保存 LoadHandle, 否则加载被取消
// ------------------------------------------------------------
void AHeightFieldViewer::StartAsyncLoad(const FSoftObjectPath& AssetPath)
{
    check(IsInGameThread());

    UE_LOG(LogCap1Viewer, Display,
        TEXT("[Cap1][Step 2] GameThread: FStreamableManager async load requested"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3 (模块 E): 调用 RequestAsyncLoad
    // 注意: Lambda 必须按值捕获 this (但 this 是 UObject, GC 可能回收!)
    // 更安全做法: 捕获 TWeakObjectPtr<AHeightFieldViewer> 并在回调里检查 IsValid()
    // ══════════════════════════════════════════════════════

    // 骨架: 异步加载 (取消注释并完成)
    // FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    // TWeakObjectPtr<AHeightFieldViewer> WeakThis(this);
    // LoadHandle = Streamable.RequestAsyncLoad(
    //     AssetPath,
    //     FStreamableDelegate::CreateLambda([WeakThis]()
    //     {
    //         if (AHeightFieldViewer* Viewer = WeakThis.Get())
    //         {
    //             Viewer->OnAssetLoaded();
    //         }
    //     })
    // );

    // 骨架阶段: 直接调用 OnAssetLoaded 以测试后续管线
    OnAssetLoaded();
}

// ------------------------------------------------------------
// OnAssetLoaded: FStreamableManager 回调 (GameThread)
// Step 3: 接收加载完成通知, 启动 UE::Tasks worker
//
// 三次线程跨越起点:
//   此函数在 GameThread 执行 (Step 3)
//   UE::Tasks::Launch 投递 worker (Step 4) — 跨越 1
//   worker 完成后 AsyncTask 回 GameThread — 跨越 2
//   GameThread 调 ENQUEUE_RENDER_COMMAND → RenderThread — 跨越 3
// ------------------------------------------------------------
void AHeightFieldViewer::OnAssetLoaded()
{
    check(IsInGameThread());

    UE_LOG(LogCap1Viewer, Display,
        TEXT("[Cap1][Step 3] GameThread: Load callback received, dispatching UE::Tasks worker"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4 (模块 D): 将 Asset 赋给 UPROPERTY 成员 (GC 接管)
    // RemoveFromRoot 后由 UPROPERTY TObjectPtr 保持 GC 可见性
    // 参考: 13-结课项目1 §七 坑2
    // ══════════════════════════════════════════════════════

    // 骨架: 实际使用时从 LoadHandle->GetLoadedAsset() 获取
    // UHeightFieldAsset* Loaded = Cast<UHeightFieldAsset>(LoadHandle->GetLoadedAsset());
    // if (Loaded)
    // {
    //     Loaded->RemoveFromRoot(); // 之前 AddToRoot 防 GC, 现在转由 UPROPERTY 持有
    //     HeightFieldAsset = Loaded;
    // }

    // 骨架阶段: 创建测试 Asset
    if (!HeightFieldAsset)
    {
        HeightFieldAsset = NewObject<UHeightFieldAsset>(GetTransientPackage());
        HeightFieldAsset->GenerateTestData(128);
    }

    // 提前提取 CPU 数据 (不能在 worker 里持有 UObject 指针)
    // 模块 F 坑7: 不能在 UE::Tasks worker 里访问 UObject 成员
    const int32 Res = HeightFieldAsset->Resolution;
    TArray<uint16> HeightsCopy = HeightFieldAsset->Heights; // 按值拷贝

    // ══════════════════════════════════════════════════════
    // [Thread Crossing 1] GameThread → UE::Tasks worker
    // 参考: Engine/Source/Runtime/Core/Public/Tasks/Task.h
    // 坑7: worker lambda 内不能持有 UObject 指针 (已提前提取 HeightsCopy)
    // ══════════════════════════════════════════════════════
    UE::Tasks::Launch(TEXT("Cap1HeightFieldPreprocess"),
        [Res, HeightsCopy = MoveTemp(HeightsCopy)]() mutable
        {
            // Step 4: Worker 线程执行 CPU 预处理
            check(!IsInGameThread());
            UE_LOG(LogCap1Viewer, Display,
                TEXT("[Cap1][Step 4] Worker: CPU preprocess started (normalize + normal estimate)"));

            // ══════════════════════════════════════════════
            // TODO [必做] 5 (模块 C): 创建 TSharedPtr 中间结果
            // 用 MakeShared (ThreadSafe) 保证跨线程引用计数安全
            // ══════════════════════════════════════════════
            auto PreprocessResult = MakeShared<FHeightFieldPreprocessResult, ESPMode::ThreadSafe>();
            PreprocessResult->Resolution = Res;
            PreprocessResult->NormalizedHeights.SetNumUninitialized(Res * Res);
            PreprocessResult->EstimatedNormals.SetNumUninitialized(Res * Res);

            // TODO [必做] 5a: 归一化 uint16 → float [0, 1]
            for (int32 i = 0; i < HeightsCopy.Num(); ++i)
            {
                PreprocessResult->NormalizedHeights[i] = HeightsCopy[i] / 65535.f;
            }

            // TODO [必做] 5b: 估算法线 (简单中心差分)
            for (int32 Y = 0; Y < Res; ++Y)
            {
                for (int32 X = 0; X < Res; ++X)
                {
                    int32 XL = FMath::Max(0, X - 1);
                    int32 XR = FMath::Min(Res - 1, X + 1);
                    int32 YD = FMath::Max(0, Y - 1);
                    int32 YU = FMath::Min(Res - 1, Y + 1);
                    float DX = PreprocessResult->NormalizedHeights[Y * Res + XR]
                             - PreprocessResult->NormalizedHeights[Y * Res + XL];
                    float DY = PreprocessResult->NormalizedHeights[YU * Res + X]
                             - PreprocessResult->NormalizedHeights[YD * Res + X];
                    FVector3f N = FVector3f(-DX, -DY, 0.1f);
                    N.Normalize();
                    PreprocessResult->EstimatedNormals[Y * Res + X] = N;
                }
            }

            UE_LOG(LogCap1Viewer, Display,
                TEXT("[Cap1][Step 5] Worker: CPU preprocess done, enqueuing render command"));

            // ══════════════════════════════════════════════
            // [Thread Crossing 2] UE::Tasks worker → GameThread
            // 先跳回 GameThread, 再从 GameThread 投递 ENQUEUE_RENDER_COMMAND
            // 原因: 避免在 worker 内直接 ENQUEUE 导致命令顺序不定
            // 参考: 13-结课项目1 §3.1 线程约束
            // ══════════════════════════════════════════════
            AsyncTask(ENamedThreads::GameThread,
                [PreprocessResult]()
                {
                    check(IsInGameThread());

                    // ══════════════════════════════════
                    // [Thread Crossing 3] GameThread → RenderThread
                    // 参考: Engine/Source/Runtime/RenderCore/Public/RenderingThread.h
                    // 坑1: lambda 必须按值捕获, 不能按引用捕获 GameThread 变量
                    // ══════════════════════════════════
                    ENQUEUE_RENDER_COMMAND(Cap1RHIUpload)(
                        [PreprocessResult](FRHICommandListImmediate& RHICmdList)
                        {
                            check(IsInRenderingThread());
                            const int32 R = PreprocessResult->Resolution;

                            // ══════════════════════════════
                            // TODO [必做] 6 (模块 G): Step 6 — RHI 纹理上传
                            // 参考: Engine/Source/Runtime/RHI/Public/RHICommandList.h
                            // ══════════════════════════════
                            UE_LOG(LogCap1Viewer, Display,
                                TEXT("[Cap1][Step 6] RenderThread: RHI texture upload started (R32_FLOAT, %dx%d)"),
                                R, R);

                            // 骨架: RHI 纹理创建 (取消注释并完成)
                            // FRHITextureCreateDesc Desc =
                            //     FRHITextureCreateDesc::Create2D(
                            //         TEXT("HeightFieldTex"),
                            //         R, R,
                            //         PF_R32_FLOAT)
                            //     .SetFlags(ETextureCreateFlags::ShaderResource);
                            // TRefCountPtr<FRHITexture> HeightFieldTex = RHICreateTexture(Desc);
                            //
                            // // 上传 CPU float 数组到 GPU 纹理
                            // uint32 Stride = 0;
                            // void* MipData = RHICmdList.LockTexture2D(
                            //     (FRHITexture2D*)HeightFieldTex.GetReference(),
                            //     0, RLM_WriteOnly, Stride, false);
                            // FMemory::Memcpy(MipData,
                            //     PreprocessResult->NormalizedHeights.GetData(),
                            //     R * R * sizeof(float));
                            // RHICmdList.UnlockTexture2D(
                            //     (FRHITexture2D*)HeightFieldTex.GetReference(), 0, false);

                            UE_LOG(LogCap1Viewer, Display,
                                TEXT("[Cap1][Step 7] RenderThread: RHI texture upload done"));

                            // ══════════════════════════════
                            // TODO [必做] 7 (模块 H): Step 8-9 — RDG 三 pass 流水线
                            // 参考: Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h
                            // 约束 §3.4: 每 pass 必须有 BEGIN_SHADER_PARAMETER_STRUCT
                            // ══════════════════════════════
                            UE_LOG(LogCap1Viewer, Display,
                                TEXT("[Cap1][Step 8] RenderThread: FRDGBuilder pass 1/2/3 added"));

                            // 骨架: RDG Builder (取消注释并完成)
                            // FRDGBuilder GraphBuilder(RHICmdList);
                            //
                            // // Pass 1: HeightField 采样 → 中间 RT
                            // // BEGIN_SHADER_PARAMETER_STRUCT(FPass1Params, )
                            // //     SHADER_PARAMETER_RDG_TEXTURE(Texture2D, HeightFieldTex)
                            // //     RENDER_TARGET_BINDING_SLOTS()
                            // // END_SHADER_PARAMETER_STRUCT()
                            //
                            // // Pass 2: 法线生成
                            // // Pass 3: Gamma 校正 → 最终 RT
                            //
                            // UE_LOG(LogCap1Viewer, Display,
                            //     TEXT("[Cap1][Step 9] RenderThread: FRDGBuilder::Execute() called"));
                            // GraphBuilder.Execute();
                            //
                            // // Step 10: PNG dump (AsyncTask 回 GameThread)
                            // AsyncTask(ENamedThreads::GameThread, []()
                            // {
                            //     UE_LOG(LogCap1Viewer, Display,
                            //         TEXT("[Cap1][Step 10] GameThread: PNG dump triggered"));
                            // });

                            UE_LOG(LogCap1Viewer, Display,
                                TEXT("[Cap1][Step 9] RenderThread: FRDGBuilder::Execute() called (骨架占位)"));
                        }
                    ); // end ENQUEUE_RENDER_COMMAND

                    UE_LOG(LogCap1Viewer, Display,
                        TEXT("[Cap1][Step 10] GameThread: PNG dump triggered (骨架占位)"));
                }
            ); // end AsyncTask GameThread
        }
    ); // end UE::Tasks::Launch
}

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 实现完整 RDG 三 pass 流水线
//   Pass1: 外部 FRHITexture → FRDGExternalTexture → 采样写入 transient R16F
//   Pass2: transient R16F → 法线生成 → transient RGBA8
//   Pass3: transient RGBA8 → Gamma 校正 → 最终 RGBA8
// 每 pass 用 BEGIN_SHADER_PARAMETER_STRUCT / END_SHADER_PARAMETER_STRUCT
// 参考: 13-结课项目1 §七 坑3
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 2: 实现 RHI Readback + PNG dump
//   在 Execute 后安排 AddReadbackPass
//   回 GameThread 后用 FImageWrapperModule + FFileHelper::SaveArrayToFile
//   落盘路径: <ProjectDir>/Saved/Cap1/pass1_heightfield.png
// 参考: 13-结课项目1 §4.3 PNG 截图 路径 A
// ══════════════════════════════════════════════════════

// ---- 验证区 ----
// (完成 TODO 后把下列 ensure 搬入对应代码路径)
// ensureMsgf(!IsInRenderingThread() || IsInGameThread(),
//     TEXT("Step 10 PNG dump 必须在 GameThread 执行"));
