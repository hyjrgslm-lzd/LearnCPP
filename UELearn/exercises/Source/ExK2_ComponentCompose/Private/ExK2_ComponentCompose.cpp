// ============================================================
// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-2
// C++ 标准要求: C++20
// 本题目标: 掌握 USceneComponent transform 附着层级 + 运行时 RegisterComponent
//
// 骨架阶段预期行为:
//   构造函数: Root + Mesh 创建，Mesh SetupAttachment(Root)
//   BeginPlay: 打印 Root/Mesh 世界坐标，动态添加 DynamicChild 并注册
//
// 完成后预期日志（Output Log 过滤 LogExK2）:
//   LogExK2: [ExK2] 构造函数 — Root/Mesh 已创建，GetWorld()=null（CDO）
//   LogExK2: [ExK2] BeginPlay — Root 世界坐标: (0,0,0)
//   LogExK2: [ExK2] BeginPlay — DynamicChild IsRegistered: true
// ============================================================

#include "ExK2_ComponentCompose.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExK2, Log, All);

IMPLEMENT_MODULE(FDefaultModuleImpl, ExK2_ComponentCompose);

// ────────────────────────────────────────────────────────────────────────────
// 构造函数 — 只能在这里调用 CreateDefaultSubobject
// ────────────────────────────────────────────────────────────────────────────
AExK2Pawn::AExK2Pawn()
{
    PrimaryActorTick.bCanEverTick = false;  // 本题不需要 Tick

    // ── 创建 Root SceneComponent 并设为 RootComponent ────────────────────
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    // ── 创建 StaticMeshComponent，附着到 Root ────────────────────────────
    // SetupAttachment 适用于构造期（Component 尚未注册），只设置 AttachParent 指针
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Root);

    UE_LOG(LogExK2, Log, TEXT("[ExK2] 构造函数 — Root/Mesh 已创建，GetWorld()=%s"),
        GetWorld() ? TEXT("有效") : TEXT("null（CDO 阶段）"));
}

// ────────────────────────────────────────────────────────────────────────────
// BeginPlay — Gameplay 开始，演示运行时动态添加 Component
// ────────────────────────────────────────────────────────────────────────────
void AExK2Pawn::BeginPlay()
{
    Super::BeginPlay();

    // ── 打印 Root 世界坐标（即 Actor 世界坐标）──────────────────────────
    UE_LOG(LogExK2, Log, TEXT("[ExK2] BeginPlay — Root 世界坐标: %s"),
        *Root->GetComponentLocation().ToString());
    UE_LOG(LogExK2, Log, TEXT("[ExK2] BeginPlay — GetActorLocation: %s"),
        *GetActorLocation().ToString());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 把 Mesh 相对偏移设为 (100, 0, 0)，打印 Mesh 世界坐标
    // Mesh->SetRelativeLocation(FVector(100.f, 0.f, 0.f));
    // UE_LOG(LogExK2, Log, TEXT("[ExK2] Mesh 世界坐标: %s"), *Mesh->GetComponentLocation().ToString());
    // ══════════════════════════════════════════════════════

    // ── 运行时动态添加 Component ─────────────────────────────────────────
    // NewObject 只在内存创建对象，Outer = this Actor
    DynamicChild = NewObject<USceneComponent>(this, TEXT("DynamicChild"));

    // SetupAttachment 在运行期也可调用，但 AttachToComponent 语义更完整（含 transform 更新）
    // 此处演示 SetupAttachment，完成 TODO 进阶后改为 AttachToComponent
    DynamicChild->SetupAttachment(Root);

    // RegisterComponent 将 Component 注册到 World，之后 BeginPlay/Tick 才会被调用
    DynamicChild->RegisterComponent();

    UE_LOG(LogExK2, Log, TEXT("[ExK2] BeginPlay — DynamicChild IsRegistered: %s"),
        DynamicChild->IsRegistered() ? TEXT("true") : TEXT("false"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 打印 DynamicChild->GetComponentLocation()
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 把 SetupAttachment 改为 AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform)
    //              观察两者在已注册状态下的行为差异
    // ══════════════════════════════════════════════════════
}

// ────────────────────────────────────────────────────────────────────────────
// EndPlay — 显式销毁运行时添加的动态 Component
// ────────────────────────────────────────────────────────────────────────────
void AExK2Pawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2: 调用 DynamicChild->DestroyComponent()，
    //              观察 OnUnregister 回调（在派生类里 override 并打印）
    // if (DynamicChild && DynamicChild->IsRegistered())
    // {
    //     DynamicChild->DestroyComponent();
    // }
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExK2, Log, TEXT("[ExK2] EndPlay"));

    Super::EndPlay(EndPlayReason);
}

// ---- 验证区 ----
// ensureMsgf(Root != nullptr, TEXT("Root 应在构造函数里创建"));
// ensureMsgf(Mesh->GetAttachParent() == Root, TEXT("Mesh 应附着到 Root"));
// ensureMsgf(DynamicChild == nullptr || DynamicChild->IsRegistered(), TEXT("动态 Component 注册失败"));
