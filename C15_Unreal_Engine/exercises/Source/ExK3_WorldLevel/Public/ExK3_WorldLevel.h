// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-3
// 小节: UWorld / ULevel / UGameInstance 三层关系 + Outer 链遍历
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExK3_WorldLevel.generated.h"

/**
 * AExK3Inspector — 在 BeginPlay 里遍历 World/Level/Actors 并打印 Outer 链。
 *
 * 骨架阶段演示:
 *   1. GetWorld()->GetCurrentLevel() 取当前 Level
 *   2. 遍历 GetWorld()->GetLevels()，打印每个 ULevel 的 Actors.Num()
 *   3. 沿 GetOuter()->GetOuter()... 链向上走直到 UPackage，打印每层类名
 */
UCLASS(BlueprintType, Blueprintable)
class EXK3_WORLDLEVEL_API AExK3Inspector : public AActor
{
    GENERATED_BODY()

public:
    AExK3Inspector();

    virtual void BeginPlay() override;

    // ── TODO [必做] 1 ────────────────────────────────────────────────────
    // 在 BeginPlay 里调用 GetWorld()->GetCurrentLevel()，
    // 打印 Level->GetName() 和 Level->Actors.Num()。

    // ── TODO [必做] 2 ────────────────────────────────────────────────────
    // 遍历 GetWorld()->GetLevels()（TArray<ULevel*>），
    // 对每个 Level 打印名称 + Actor 数量。

    // ── TODO [必做] 3 ────────────────────────────────────────────────────
    // 演示 Outer 链: 从 this 开始循环 GetOuter()，
    // 每步打印 GetClass()->GetName()，直到 Cast<UPackage>(Outer) != nullptr。

    // ── TODO [进阶] 1 ────────────────────────────────────────────────────
    // 阅读 LevelActor.cpp §671，验证新 Spawn 的 Actor 的 GetOuter() == ULevel*。
    // 在 BeginPlay 里 SpawnActor<AActor>() 然后 check(Spawned->GetOuter() == GetLevel())。
};
