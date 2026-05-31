// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-1
// 小节: AActor 生命周期五钩子 — PostInitializeComponents / BeginPlay / Tick / EndPlay / Destroyed
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExK1_HelloActor.generated.h"

/**
 * AExK1HelloActor — 演示 AActor 完整生命周期钩子序列。
 *
 * 骨架阶段：五个钩子各打印一条 UE_LOG，构造函数同样打印，
 * 方便在 Output Log 中观察调用顺序。
 */
UCLASS(BlueprintType, Blueprintable)
class EXK1_HELLOACTOR_API AExK1HelloActor : public AActor
{
    GENERATED_BODY()

public:
    AExK1HelloActor();

    // ── 生命周期钩子 override ───────────────────────────────────────────
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;

    // ── TODO [必做] 1 ───────────────────────────────────────────────────
    // 在 BeginPlay 里额外打印 GetWorld() != nullptr（证明此时 World 有效）。
    // 已在 .cpp 骨架中留有注释位置。

    // ── TODO [必做] 2 ───────────────────────────────────────────────────
    // 在 EndPlay 里用 UEnum::GetValueAsString(EndPlayReason) 打印触发原因。
    // 已在 .cpp 骨架中留有注释位置。

    // ── TODO [进阶] 1 ───────────────────────────────────────────────────
    // override OnConstruction(const FTransform& Transform)，
    // 在 Editor 中拖动 Actor 时观察 Construction Script 重新执行。
    // virtual void OnConstruction(const FTransform& Transform) override;
};
