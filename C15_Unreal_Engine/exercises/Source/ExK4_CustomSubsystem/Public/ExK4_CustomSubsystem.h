// 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-4
// 小节: 三层 Subsystem 生命周期 — EngineSubsystem / GameInstanceSubsystem / WorldSubsystem
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "ExK4_CustomSubsystem.generated.h"

// ────────────────────────────────────────────────────────────────────────────
// 1. Engine 层 Subsystem — 生命周期绑定 GEngine，整个 Editor 进程只初始化一次
// ────────────────────────────────────────────────────────────────────────────
UCLASS()
class EXK4_CUSTOMSUBSYSTEM_API UExK4EngineSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // TODO [必做]: 多次开始/停止 PIE，确认 Initialize 只在 Editor 启动时调用一次
};

// ────────────────────────────────────────────────────────────────────────────
// 2. GameInstance 层 Subsystem — 生命周期绑定 UGameInstance，每次 PIE 重建
// ────────────────────────────────────────────────────────────────────────────
UCLASS()
class EXK4_CUSTOMSUBSYSTEM_API UExK4GameInstanceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 跨关卡计数器示例
    int32 GetPlayCount() const { return PlayCount; }
    void  IncrementPlayCount() { ++PlayCount; }

private:
    // 注意: 普通 int32 不需要 UPROPERTY，但若改为 UObject* 则必须加
    int32 PlayCount = 0;

    // TODO [必做]: 在 BeginPlay Actor 里用 GetGameInstance()->GetSubsystem<UExK4GameInstanceSubsystem>()
    // 调用 IncrementPlayCount()，观察 PIE 重启后 PlayCount 从 0 重置
};

// ────────────────────────────────────────────────────────────────────────────
// 3. World 层 Subsystem — 生命周期绑定 UWorld，PIE Players=2 时各自独立实例
// ────────────────────────────────────────────────────────────────────────────
UCLASS()
class EXK4_CUSTOMSUBSYSTEM_API UExK4WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // UWorldSubsystem 额外提供以下生命周期钩子（可选 override）:
    // virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    // virtual void OnWorldEndPlay(UWorld& InWorld) override;

    // TODO [进阶]: override DoesSupportWorldType，对 EWorldType::Editor 返回 false
    // virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
