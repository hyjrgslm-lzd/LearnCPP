// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-3
// 小节: FStreamableManager::RequestAsyncLoad + FStreamableHandle 生命周期
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "ExE3_Streamable.generated.h"

/**
 * AExE3StreamActor — 在 BeginPlay 里发起异步加载，在 CompleteDelegate lambda 里取结果。
 *
 * 设计说明:
 *   - FStreamableManager 是非 UObject，以成员变量形式持有（也可用 UAssetManager::GetStreamableManager()）
 *   - TSharedPtr<FStreamableHandle> 用普通成员存储（不能用 UPROPERTY），
 *     bManageActiveHandle=true 让 Manager 内部也保住 Handle 防止提前析构
 *   - CompleteDelegate 在 GameThread 上被调用，lambda 里用 TWeakObjectPtr 捕获 this
 */
UCLASS(BlueprintType, Blueprintable)
class EXE3_STREAMABLE_API AExE3StreamActor : public AActor
{
    GENERATED_BODY()

public:
    AExE3StreamActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ── TODO [必做] 1: 修改下面的路径为你工程中真实存在的资产路径 ──────────
    // 格式: /Game/PackagePath/AssetName.AssetName（注意点号分隔）
    UPROPERTY(EditAnywhere, Category = "E3")
    FSoftObjectPath AssetToLoad = FSoftObjectPath(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));

    // ── TODO [必做] 2: 在 CompleteDelegate 里打印 GetLoadedAsset()->GetName() ──
    // ── TODO [必做] 3: 在 RequestAsyncLoad 后立即打印 HasLoadCompleted() ────────
    // ── TODO [必做] 4: 试验 CancelHandle()，观察 CompleteDelegate 不被触发 ───────

    // ── TODO [进阶] 1: 改用 AsyncLoadHighPriority，对比完成帧数 ─────────────────
    // ── TODO [进阶] 2: bManageActiveHandle=false，局部 TSharedPtr 超出作用域后观察行为 ─

private:
    // FStreamableManager 是非 UObject，直接作成员，也可使用 UAssetManager::GetStreamableManager()
    FStreamableManager StreamableManager;

    // Handle 生命周期: bManageActiveHandle=true 时 Manager 内部也保住一份 TSharedRef
    TSharedPtr<FStreamableHandle> StreamableHandle;
};
