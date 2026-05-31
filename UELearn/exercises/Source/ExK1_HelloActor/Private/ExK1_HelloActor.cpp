// ============================================================
// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-1
// C++ 标准要求: C++20
// 本题目标: 观察 AActor 从 Spawn 到销毁的五钩子调用顺序
//
// 骨架阶段预期行为: 模块注册成功，PIE 启动后 Output Log 出现 [ExK1] 系列日志
// 完成后预期日志顺序（Output Log 过滤 LogExK1）:
//   LogExK1: [ExK1] 构造函数 — GetWorld() 此时通常为 null（CDO 阶段）
//   LogExK1: [ExK1] PostInitializeComponents — 所有 Component 已注册
//   LogExK1: [ExK1] BeginPlay — World 有效: true
//   LogExK1: [ExK1] Tick — DeltaSeconds=...（每帧，bCanEverTick=true）
//   LogExK1: [ExK1] EndPlay — 原因: EndPlayInEditor
//   LogExK1: [ExK1] Destroyed
// ============================================================

#include "ExK1_HelloActor.h"
#include "Modules/ModuleManager.h"

// 每题独立 log category，Output Log 里可按 LogExK1 过滤
DEFINE_LOG_CATEGORY_STATIC(LogExK1, Log, All);

IMPLEMENT_MODULE(FDefaultModuleImpl, ExK1_HelloActor);

// ────────────────────────────────────────────────────────────────────────────
// 构造函数
// ────────────────────────────────────────────────────────────────────────────
AExK1HelloActor::AExK1HelloActor()
{
    // Tick 必须在构造函数里开启，否则 Tick() 永远不被调用
    PrimaryActorTick.bCanEverTick = true;

    UE_LOG(LogExK1, Log, TEXT("[ExK1] 构造函数 — 此时 GetWorld()=%s"),
        GetWorld() ? TEXT("有效") : TEXT("null（CDO 或 Editor 持久 World）"));
}

// ────────────────────────────────────────────────────────────────────────────
// 钩子 1: PostInitializeComponents — 所有 Component 已 RegisterComponent
// ────────────────────────────────────────────────────────────────────────────
void AExK1HelloActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();  // 必须调用，否则 Component 初始化链断裂

    UE_LOG(LogExK1, Log, TEXT("[ExK1] PostInitializeComponents — 所有 Component 已注册"));
}

// ────────────────────────────────────────────────────────────────────────────
// 钩子 2: BeginPlay — Gameplay 真正开始，World 保证有效
// ────────────────────────────────────────────────────────────────────────────
void AExK1HelloActor::BeginPlay()
{
    Super::BeginPlay();  // 内部调用所有 Component 的 BeginPlay（Actor.cpp:4764）

    UE_LOG(LogExK1, Log, TEXT("[ExK1] BeginPlay — World 有效: %s"),
        GetWorld() != nullptr ? TEXT("true") : TEXT("false"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 打印 GetActorLocation() 初始位置
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 调用 GetNetMode()，记录 NM_Standalone / NM_ListenServer / NM_Client
    // ══════════════════════════════════════════════════════
}

// ────────────────────────────────────────────────────────────────────────────
// 钩子 3: Tick — 每帧 GameThread 调用（仅当 bCanEverTick=true）
// ────────────────────────────────────────────────────────────────────────────
void AExK1HelloActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 骨架阶段: 每帧打印会刷屏，建议完成 TODO 后改为条件输出
    UE_LOG(LogExK1, Verbose, TEXT("[ExK1] Tick — DeltaSeconds=%.4f"), DeltaSeconds);
}

// ────────────────────────────────────────────────────────────────────────────
// 钩子 4: EndPlay — Actor 停止参与游戏逻辑（PIE 停止 / Destroy() 均触发）
// ────────────────────────────────────────────────────────────────────────────
void AExK1HelloActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 用 UEnum::GetValueAsString(EndPlayReason) 打印触发原因
    // 示例: UEnum::GetValueAsString(EndPlayReason)
    // ══════════════════════════════════════════════════════
    UE_LOG(LogExK1, Log, TEXT("[ExK1] EndPlay — EndPlayReason=%d（完成 TODO 2 后换成枚举字符串）"),
        static_cast<int32>(EndPlayReason));

    Super::EndPlay(EndPlayReason);  // 路由 Blueprint 侧 ReceiveEndPlay
}

// ────────────────────────────────────────────────────────────────────────────
// 钩子 5: Destroyed — Actor 从 World 移除前的最后广播点
//   内部: RouteEndPlay(EEndPlayReason::Destroyed) → OnDestroyed delegate → ReceiveDestroyed
// ────────────────────────────────────────────────────────────────────────────
void AExK1HelloActor::Destroyed()
{
    UE_LOG(LogExK1, Log, TEXT("[ExK1] Destroyed — Actor 即将从 World 移除"));

    Super::Destroyed();
}

// ---- 验证区（完成 TODO 后把下列 ensure 搬入对应代码路径）----
// ensureMsgf(GetWorld() != nullptr, TEXT("BeginPlay 里 World 应有效"));
// ensureMsgf(PrimaryActorTick.bCanEverTick == true, TEXT("Tick 应已开启"));
