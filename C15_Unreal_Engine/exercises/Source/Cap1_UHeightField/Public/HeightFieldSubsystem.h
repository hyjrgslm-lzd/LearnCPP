// 对应章节: ../../../13-结课项目1-集成项目.md §模块K: UWorldSubsystem 管理批处理
// 小节: UHeightFieldWorldSubsystem — 管理批量加载队列 + Console 命令注册
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
// 参考: Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h
#include "HeightFieldSubsystem.generated.h"

class AHeightFieldViewer;
class UHeightFieldAsset;

// ══════════════════════════════════════════════════════════════
// UHeightFieldWorldSubsystem: 模块 K (UWorldSubsystem) 知识点
// - 继承 UWorldSubsystem: 生命周期绑定 UWorld, 框架自动 Initialize/Deinitialize
// - 无需手动 NewObject: Subsystem 框架通过反射自动发现并实例化
// - 批处理队列放在 Subsystem 里而非全局变量 (防止 PIE 多次开启时悬空引用)
// 约束 (§3.3): 必须继承 UWorldSubsystem, 实现 Initialize/Deinitialize
// ══════════════════════════════════════════════════════════════

UCLASS()
class CAP1_UHEIGHTFIELD_API UHeightFieldWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ──────────────────────────────────────────────────────────
    // UWorldSubsystem 生命周期钩子
    // Initialize:      World 创建后立即调用, 注册 Console 命令
    // Deinitialize:    World 销毁前调用, 清理队列和资源
    // OnWorldBeginPlay: World BeginPlay 后调用, 安全进行 SpawnActor
    // ──────────────────────────────────────────────────────────
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 1: Console 命令处理函数
    // 命令: UELearn.Cap1.Run <size> <resolution>
    // 注册时机: Initialize 内 (此时 World 已存在)
    // 参考: 13-结课项目1 §4.1 Console 命令触发
    // ──────────────────────────────────────────────────────────
    void HandleRunCommand(const TArray<FString>& Args);

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 2: 批量提交接口
    // 将 FSoftObjectPath 加入 BatchQueue, 调度异步加载
    // ──────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Cap1")
    void SubmitBatch(const TArray<FSoftObjectPath>& Paths);

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 3: 触发完整管线
    // 内部流程:
    //   Step 1: NewObject<UHeightFieldAsset>(GetTransientPackage())
    //   Step 2: RequestAsyncLoad (FSoftObjectPath)
    //   Step 3: 加载回调 → 赋给 AHeightFieldViewer 的 UPROPERTY
    // 参考: 13-结课项目1 §九 Day1 步骤 7-9
    // ──────────────────────────────────────────────────────────
    void RunPipeline(int32 InSize, int32 InResolution);

private:
    // ──────────────────────────────────────────────────────────
    // 批处理队列 (非 UObject, 不需要 UPROPERTY GC 追踪)
    // ──────────────────────────────────────────────────────────
    TArray<FSoftObjectPath> BatchQueue;

    // ──────────────────────────────────────────────────────────
    // AHeightFieldViewer 弱引用 (Actor 由 World 管理生命周期)
    // 使用 TWeakObjectPtr 避免循环引用 + 处理 stale 情况
    // 约束 §3.2: 禁止裸 UObject* 成员变量
    // ──────────────────────────────────────────────────────────
    TWeakObjectPtr<AHeightFieldViewer> ViewerActor;

    // Console 命令句柄 (Deinitialize 时需要注销)
    IConsoleCommand* RunCommandHandle;
};
