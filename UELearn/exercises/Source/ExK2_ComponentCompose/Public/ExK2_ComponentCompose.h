// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-2
// 小节: USceneComponent transform 附着层级 + 运行时动态添加 Component
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExK2_ComponentCompose.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * AExK2Pawn — 演示 SceneComponent 层级组合。
 *
 * 构造函数里创建:
 *   Root (USceneComponent)  ← SetRootComponent
 *     └── Mesh (UStaticMeshComponent) ← SetupAttachment(Root)
 *
 * BeginPlay 里演示运行时 NewObject + RegisterComponent 动态添加子组件。
 */
UCLASS(BlueprintType, Blueprintable)
class EXK2_COMPONENTCOMPOSE_API AExK2Pawn : public AActor
{
    GENERATED_BODY()

public:
    AExK2Pawn();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
    // ── 构造期 Component（UPROPERTY 保证 GC 可见）──────────────────────
    UPROPERTY(VisibleAnywhere, Category = "K2|Components")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category = "K2|Components")
    TObjectPtr<UStaticMeshComponent> Mesh;

    // ── 运行时动态添加的 Component（UPROPERTY 保证 GC 可见）──────────────
    UPROPERTY(Transient, VisibleAnywhere, Category = "K2|Components")
    TObjectPtr<USceneComponent> DynamicChild;

    // ── TODO [必做] 1 ────────────────────────────────────────────────────
    // 在 BeginPlay 里打印 Root->GetComponentLocation() 和 GetActorLocation()，
    // 验证两者一致（Actor transform 由 RootComponent 代理）。

    // ── TODO [必做] 2 ────────────────────────────────────────────────────
    // 在 BeginPlay 里用 NewObject<USceneComponent>(this, TEXT("DynamicChild"))
    // + SetupAttachment(Root) + RegisterComponent()，打印 IsRegistered()。

    // ── TODO [进阶] 1 ────────────────────────────────────────────────────
    // 在 EndPlay 里调用 DynamicChild->DestroyComponent()，
    // override OnUnregister() 并打印，观察卸载时机。
};
