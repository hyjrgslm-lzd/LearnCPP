// ============================================================
// 对应章节: ../../../13-结课项目1-集成项目.md §模块K
// C++ 标准要求: C++20
// 本文件: UHeightFieldWorldSubsystem 实现
//
// 模块 K 知识点:
//   - UWorldSubsystem 生命周期: Initialize → OnWorldBeginPlay → Deinitialize
//   - Initialize 时 World 存在但 BeginPlay 未触发, 不能在此 SpawnActor
//   - OnWorldBeginPlay 里才能安全 SpawnActor
//   - Subsystem 框架通过反射自动发现所有 UWorldSubsystem 子类, 无需手动注册
// ============================================================

#include "HeightFieldSubsystem.h"
#include "AHeightFieldViewer.h"
#include "UHeightFieldAsset.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCap1Subsystem, Log, All);

// ------------------------------------------------------------
// Initialize: World 创建后立即调用
// 注册 Console 命令 (此时 World 已存在, 命令注册安全)
// 注意: 不要在此 SpawnActor (World 的 BeginPlay 还未触发)
// 参考: 13-结课项目1 §七 坑5: Initialize 时 World 未 BeginPlay
// ------------------------------------------------------------
void UHeightFieldWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    RunCommandHandle = nullptr;
    ViewerActor = nullptr;

    UE_LOG(LogCap1Subsystem, Log, TEXT("[Cap1] UHeightFieldWorldSubsystem::Initialize"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 注册 Console 命令
    // 命令格式: UELearn.Cap1.Run <size> <resolution>
    // 注册时机: Initialize 内 (World 已存在, 但 BeginPlay 未触发)
    // ══════════════════════════════════════════════════════

    // 骨架: Console 命令注册 (取消注释并实现)
    // RunCommandHandle = IConsoleManager::Get().RegisterConsoleCommand(
    //     TEXT("UELearn.Cap1.Run"),
    //     TEXT("Run Cap1 UHeightField pipeline. Args: <size> <resolution>"),
    //     FConsoleCommandWithArgsDelegate::CreateUObject(
    //         this, &UHeightFieldWorldSubsystem::HandleRunCommand),
    //     ECVF_Default
    // );
}

// ------------------------------------------------------------
// Deinitialize: World 销毁前调用, 清理资源
// ------------------------------------------------------------
void UHeightFieldWorldSubsystem::Deinitialize()
{
    UE_LOG(LogCap1Subsystem, Log, TEXT("[Cap1] UHeightFieldWorldSubsystem::Deinitialize"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 注销 Console 命令
    // ══════════════════════════════════════════════════════
    if (RunCommandHandle)
    {
        IConsoleManager::Get().UnregisterConsoleObject(RunCommandHandle);
        RunCommandHandle = nullptr;
    }

    BatchQueue.Empty();
    ViewerActor = nullptr;

    Super::Deinitialize();
}

// ------------------------------------------------------------
// OnWorldBeginPlay: World BeginPlay 后调用
// 在此 SpawnActor 安全 (Level 已就绪)
// 参考: 13-结课项目1 §七 坑5 正确做法
// ------------------------------------------------------------
void UHeightFieldWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    UE_LOG(LogCap1Subsystem, Log, TEXT("[Cap1] UHeightFieldWorldSubsystem::OnWorldBeginPlay"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 在此 SpawnActor<AHeightFieldViewer>
    // 注意: 不要在 Initialize 里 SpawnActor
    // ══════════════════════════════════════════════════════

    // 骨架: SpawnActor (取消注释)
    // FActorSpawnParameters SpawnParams;
    // SpawnParams.SpawnCollisionHandlingOverride =
    //     ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // AHeightFieldViewer* Viewer = InWorld.SpawnActor<AHeightFieldViewer>(
    //     AHeightFieldViewer::StaticClass(), FTransform::Identity, SpawnParams);
    // ViewerActor = Viewer;
    // UE_LOG(LogCap1Subsystem, Log, TEXT("[Cap1] AHeightFieldViewer spawned"));
}

// ------------------------------------------------------------
// HandleRunCommand: Console 命令回调 (GameThread)
// 解析参数, 调用 RunPipeline
// ------------------------------------------------------------
void UHeightFieldWorldSubsystem::HandleRunCommand(const TArray<FString>& Args)
{
    if (Args.Num() < 2)
    {
        UE_LOG(LogCap1Subsystem, Warning,
            TEXT("[Cap1] Usage: UELearn.Cap1.Run <size> <resolution>"));
        return;
    }

    int32 InSize = FCString::Atoi(*Args[0]);
    int32 InResolution = FCString::Atoi(*Args[1]);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 验证参数合法性 (Resolution 必须是 2 的幂次)
    // ══════════════════════════════════════════════════════

    UE_LOG(LogCap1Subsystem, Log,
        TEXT("[Cap1] HandleRunCommand: size=%d resolution=%d"),
        InSize, InResolution);

    RunPipeline(InSize, InResolution);
}

// ------------------------------------------------------------
// RunPipeline: 触发完整九步管线
// 参考: 13-结课项目1 §4.2 日志时序 Step 1-10
// ------------------------------------------------------------
void UHeightFieldWorldSubsystem::RunPipeline(int32 InSize, int32 InResolution)
{
    check(IsInGameThread());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 5 (模块 D): Step 1 — 创建 HeightField Asset
    // 禁止: new UHeightFieldAsset()  (裸 new UObject 违反约束)
    // 正确: NewObject<UHeightFieldAsset>(GetTransientPackage())
    // 参考: 13-结课项目1 §七 坑2: AddToRoot 防 GC 提前回收
    // ══════════════════════════════════════════════════════

    // 骨架: 创建 Asset (取消注释并完成)
    // UHeightFieldAsset* HeightField = NewObject<UHeightFieldAsset>(GetTransientPackage());
    // HeightField->AddToRoot(); // 防止 GC 在异步加载期间回收
    // HeightField->GenerateTestData(InResolution);
    // HeightField->Size = InSize;
    // UE_LOG(LogCap1Subsystem, Display,
    //     TEXT("[Cap1][Step 1] GameThread: UHeightField asset created (size=%d, res=%d)"),
    //     InSize, InResolution);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 6 (模块 E): Step 2 — 异步加载
    // 参考: Engine/Source/Runtime/Engine/Classes/Engine/StreamableManager.h
    // 注意: 保存 handle 防止加载被取消 (坑6)
    // ══════════════════════════════════════════════════════

    // 骨架: 异步加载 (取消注释)
    // FSoftObjectPath AssetPath(HeightField);
    // UE_LOG(LogCap1Subsystem, Display,
    //     TEXT("[Cap1][Step 2] GameThread: FStreamableManager async load requested"));
    //
    // FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    // TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(
    //     AssetPath,
    //     FStreamableDelegate::CreateUObject(
    //         this, &UHeightFieldWorldSubsystem::OnHeightFieldLoaded, HeightField)
    // );
    // 将 handle 保存到 ViewerActor 或 Subsystem 成员避免提前释放

    // ══════════════════════════════════════════════════════
    // TODO [必做] 7: Step 3-10 在 AHeightFieldViewer::OnAssetLoaded 中实现
    // ══════════════════════════════════════════════════════
}

// ------------------------------------------------------------
// SubmitBatch: 批量提交异步加载路径
// ------------------------------------------------------------
void UHeightFieldWorldSubsystem::SubmitBatch(const TArray<FSoftObjectPath>& Paths)
{
    BatchQueue.Append(Paths);

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 实现批量调度逻辑
    // ══════════════════════════════════════════════════════
    UE_LOG(LogCap1Subsystem, Log,
        TEXT("[Cap1] SubmitBatch: %d paths queued, total=%d"),
        Paths.Num(), BatchQueue.Num());
}
