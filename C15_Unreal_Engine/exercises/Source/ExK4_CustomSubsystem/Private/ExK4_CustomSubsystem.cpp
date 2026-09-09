// ============================================================
// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-4
// C++ 标准要求: C++20
// 本题目标: 实现三层 Subsystem，观察各层 Initialize/Deinitialize 与 PIE 的关系
//
// 骨架阶段预期行为: 模块注册成功，PIE 启动后 Output Log 出现对应 [ExK4] 日志
//
// 完成后对比观察表（填入你的日志时间戳）:
//   UExK4EngineSubsystem::Initialize      — Editor 启动时 1 次，PIE 重启不重触发
//   UExK4GameInstanceSubsystem::Initialize — 每次 PIE 启动 1 次
//   UExK4WorldSubsystem::Initialize        — 每个 World 创建时 1 次（PIE Players=2 时 2 次）
// ============================================================

#include "ExK4_CustomSubsystem.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExK4, Log, All);

IMPLEMENT_MODULE(FDefaultModuleImpl, ExK4_CustomSubsystem);

// ────────────────────────────────────────────────────────────────────────────
// 1. UExK4EngineSubsystem
// ────────────────────────────────────────────────────────────────────────────
void UExK4EngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogExK4, Log,
        TEXT("[ExK4][EngineSubsystem] Initialize — 生命周期绑定 GEngine，整个 Editor 进程只调用一次"));
}

void UExK4EngineSubsystem::Deinitialize()
{
    UE_LOG(LogExK4, Log,
        TEXT("[ExK4][EngineSubsystem] Deinitialize — Editor 关闭时调用"));

    Super::Deinitialize();
}

// ────────────────────────────────────────────────────────────────────────────
// 2. UExK4GameInstanceSubsystem
// ────────────────────────────────────────────────────────────────────────────
void UExK4GameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogExK4, Log,
        TEXT("[ExK4][GameInstanceSubsystem] Initialize — 每次 PIE 启动时调用，PlayCount 从 0 开始"));
}

void UExK4GameInstanceSubsystem::Deinitialize()
{
    UE_LOG(LogExK4, Log,
        TEXT("[ExK4][GameInstanceSubsystem] Deinitialize — PIE 停止时调用，PlayCount 最终值: %d"),
        PlayCount);

    Super::Deinitialize();
}

// ── 访问示例（从 Actor::BeginPlay 调用）─────────────────────────────────────
// if (UGameInstance* GI = GetGameInstance())
// {
//     if (UExK4GameInstanceSubsystem* Sys = GI->GetSubsystem<UExK4GameInstanceSubsystem>())
//     {
//         Sys->IncrementPlayCount();
//         UE_LOG(LogExK4, Log, TEXT("[ExK4] PlayCount = %d"), Sys->GetPlayCount());
//     }
// }

// ────────────────────────────────────────────────────────────────────────────
// 3. UExK4WorldSubsystem
// ────────────────────────────────────────────────────────────────────────────
void UExK4WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // GetWorld() 在 Initialize 时已可用（Outer = UWorld）
    UWorld* W = GetWorld();
    UE_LOG(LogExK4, Log,
        TEXT("[ExK4][WorldSubsystem] Initialize — World: %s"),
        W ? *W->GetName() : TEXT("(null)"));

    // ══════════════════════════════════════════════════════
    // TODO [必做]: PIE Players=2 时，打开两个 PIE 窗口，
    //              观察 Server World 和 Client World 各自触发一次 Initialize
    // ══════════════════════════════════════════════════════
}

void UExK4WorldSubsystem::Deinitialize()
{
    UWorld* W = GetWorld();
    UE_LOG(LogExK4, Log,
        TEXT("[ExK4][WorldSubsystem] Deinitialize — World: %s"),
        W ? *W->GetName() : TEXT("(null)"));

    Super::Deinitialize();
}

// ══════════════════════════════════════════════════════
// TODO [进阶]: override DoesSupportWorldType，过滤 Editor World
// bool UExK4WorldSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
// {
//     // 只在 Game / PIE World 里创建实例，避免 Editor World 干扰日志
//     return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
// }
// ══════════════════════════════════════════════════════

// ---- 验证区 ----
// 在 UExK4EngineSubsystem::Initialize 里加:
// ensureMsgf(GEngine != nullptr, TEXT("EngineSubsystem 初始化时 GEngine 应有效"));
