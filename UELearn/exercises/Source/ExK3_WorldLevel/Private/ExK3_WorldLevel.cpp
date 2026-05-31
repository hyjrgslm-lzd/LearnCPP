// ============================================================
// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-3
// C++ 标准要求: C++20
// 本题目标: 遍历 World→Level→Actors 链路，打印 Outer 链直至 UPackage
//
// 骨架阶段预期行为:
//   BeginPlay 打印当前 Level 名称 + Actor 数量 + Outer 链各层类名
//
// 完成后预期日志（Output Log 过滤 LogExK3）:
//   LogExK3: [ExK3] 当前 Level: PersistentLevel, Actors 数量: N
//   LogExK3: [ExK3] Level[0]: PersistentLevel, Actors: N
//   LogExK3: [ExK3] Outer 链[0]: AExK3Inspector
//   LogExK3: [ExK3] Outer 链[1]: ULevel (PersistentLevel)
//   LogExK3: [ExK3] Outer 链[2]: UWorld (...)
//   LogExK3: [ExK3] Outer 链[3]: UPackage — 到达顶层
// ============================================================

#include "ExK3_WorldLevel.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "UObject/Package.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExK3, Log, All);

IMPLEMENT_MODULE(FDefaultModuleImpl, ExK3_WorldLevel);

AExK3Inspector::AExK3Inspector()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AExK3Inspector::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!ensureMsgf(World != nullptr, TEXT("[ExK3] BeginPlay 时 World 应有效")))
    {
        return;
    }

    // ── TODO [必做] 1: 打印当前 Level ──────────────────────────────────
    ULevel* CurrentLevel = World->GetCurrentLevel();
    if (CurrentLevel)
    {
        UE_LOG(LogExK3, Log, TEXT("[ExK3] 当前 Level: %s, Actors 数量: %d"),
            *CurrentLevel->GetName(),
            CurrentLevel->Actors.Num());
    }

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 遍历 GetWorld()->GetLevels()，打印每个 Level 的名称和 Actor 数量
    // 参考: Engine/Classes/Engine/World.h — GetLevels() 返回 TArray<ULevel*>
    // ══════════════════════════════════════════════════════
    const TArray<ULevel*>& AllLevels = World->GetLevels();
    for (int32 i = 0; i < AllLevels.Num(); ++i)
    {
        ULevel* Lv = AllLevels[i];
        if (Lv)
        {
            UE_LOG(LogExK3, Log, TEXT("[ExK3] Level[%d]: %s, Actors: %d"),
                i, *Lv->GetName(), Lv->Actors.Num());
        }
    }

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 沿 Outer 链向上爬，直到遇到 UPackage
    // 演示: Actor.Outer = ULevel, ULevel.Outer = UWorld, UWorld.Outer = UPackage (或 UGameInstance)
    // ══════════════════════════════════════════════════════
    {
        UObject* Current = this;
        int32 Depth = 0;
        while (Current)
        {
            UE_LOG(LogExK3, Log, TEXT("[ExK3] Outer 链[%d]: %s (%s)"),
                Depth,
                *Current->GetName(),
                *Current->GetClass()->GetName());

            if (Cast<UPackage>(Current))
            {
                UE_LOG(LogExK3, Log, TEXT("[ExK3] Outer 链 — 到达 UPackage，遍历结束"));
                break;
            }
            Current = Current->GetOuter();
            ++Depth;

            // 安全上限，防止意外循环
            if (Depth > 20)
            {
                UE_LOG(LogExK3, Warning, TEXT("[ExK3] Outer 链超过 20 层，强制终止"));
                break;
            }
        }
    }

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: SpawnActor<AActor>() 然后验证 Spawned->GetOuter() == GetLevel()
    // FActorSpawnParameters Params;
    // AActor* Spawned = World->SpawnActor<AActor>(AActor::StaticClass(), GetActorTransform(), Params);
    // ensureMsgf(Spawned && Spawned->GetOuter() == GetLevel(),
    //     TEXT("新 Spawn 的 Actor 的 Outer 应等于所在 ULevel"));
    // ══════════════════════════════════════════════════════
}

// ---- 验证区 ----
// ensureMsgf(GetWorld() != nullptr, TEXT("BeginPlay 里 World 应有效"));
// ensureMsgf(GetOuter() != nullptr && Cast<ULevel>(GetOuter()) != nullptr,
//     TEXT("Actor 的 Outer 应是 ULevel"));
