// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-2
// 小节: FScene 双对象镜像 — GameThread Component vs RenderThread Proxy
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "PrimitiveSceneProxy.h"
#include "ExH2_SceneProxy.generated.h"

// ============================================================
// ExH2_SceneProxy 骨架
//
// 教学重点: GameThread / RenderThread 双对象镜像
//
//   GameThread 侧 (UObject, GC 管理):
//     UExH2PrimitiveComponent : public UPrimitiveComponent
//       └─ 重写 CreateSceneProxy() → 返回 FExH2SceneProxy*
//
//   RenderThread 侧 (非 UObject, FScene 手动管理):
//     FExH2SceneProxy : public FPrimitiveSceneProxy
//       └─ 空实现 GetDynamicMeshElements (骨架阶段)
//
// 镜像同步机制:
//   UPrimitiveComponent::RegisterComponent()
//     → SendRenderState_Concurrent() [GameThread]
//     → CreateSceneProxy() [GameThread 调用, 但返回对象只在 RenderThread 使用]
//     → ENQUEUE_RENDER_COMMAND → FScene::AddPrimitive() [RenderThread]
//
// 关键: FExH2SceneProxy 构造时必须按值复制所有需要的数据,
//       不可持有 UObject* 指针 (GC 可在 GameThread 任意时刻回收)
//
// 源码校对路径:
//   Engine/Source/Runtime/Engine/Public/PrimitiveSceneProxy.h
//   Engine/Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h
// ============================================================

// ---- RenderThread 侧代理 ----

/**
 * FExH2SceneProxy
 *
 * FPrimitiveSceneProxy 子类，活在 RenderThread，由 FScene 拥有。
 * 骨架阶段只空实现 GetDynamicMeshElements，不提交任何 mesh draw command。
 *
 * 常见坑: 不要在构造函数里存 UPrimitiveComponent*；
 *         需要的数据（Transform、Bounds）在构造时按值从 Component 复制。
 */
class FExH2SceneProxy : public FPrimitiveSceneProxy
{
public:
    /**
     * 构造函数在 GameThread 上被调用 (由 CreateSceneProxy() 触发)。
     * 此处按值复制所需数据，之后该对象仅在 RenderThread 上访问。
     *
     * @param InComponent  调用方传入的 Component (仅构造期使用，不持有指针)
     */
    FExH2SceneProxy(const UPrimitiveComponent* InComponent);

    virtual ~FExH2SceneProxy() override;

    // ── RenderThread 接口 ────────────────────────────────────

    /**
     * GetDynamicMeshElements
     *
     * 每帧由 FSceneRenderer 在 RenderThread 调用，提取动态 MeshBatch。
     * 骨架阶段不提交任何 batch，仅打印一次日志演示调用路径。
     *
     * TODO [必做] 1: 向 Collector 提交一个 FMeshBatch (使用 Collector.AllocateMesh())
     *               注意: 在此函数内禁止 new/delete，必须通过 FMeshElementCollector 分配
     */
    virtual void GetDynamicMeshElements(
        const TArray<const FSceneView*>& Views,
        const FSceneViewFamily& ViewFamily,
        uint32 VisibilityMap,
        FMeshElementCollector& Collector) const override;

    /**
     * GetViewRelevance
     *
     * 声明本 Primitive 在当前 View 下的渲染路径 (Static/Dynamic/Translucent 等)。
     * 骨架阶段返回空 relevance，意味着 GetDynamicMeshElements 不会被调用。
     *
     * TODO [必做] 2: 设置 Result.bDynamicRelevance = true 使 GetDynamicMeshElements 被触发
     */
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

    /** SIZE_T 报告代理对象内存占用，供 UE 内存统计使用 */
    virtual uint32 GetMemoryFootprint() const override;

    /**
     * GetTypeHash (纯虚, 必须实现)
     *
     * FPrimitiveSceneProxy 要求每个子类返回一个类唯一的哈希值,
     * 引擎用于将 SceneProxy 与对应类型进行映射。
     * 标准做法: 返回一个 static 局部变量地址作为类唯一标识。
     */
    virtual SIZE_T GetTypeHash() const override;

private:
    // 按值从 Component 复制过来的调试标签 (演示: 不持有 UObject*)
    FString DebugLabel;
};

// ---- GameThread 侧 Component ----

/**
 * UExH2PrimitiveComponent
 *
 * UPrimitiveComponent 子类，活在 GameThread，由 GC 管理。
 * 唯一职责: 重写 CreateSceneProxy() 返回 FExH2SceneProxy 实例。
 *
 * 关联 Actor: 在 PIE 中把本 Component 添加到 Actor 即可观察双对象镜像。
 */
UCLASS(ClassGroup=(ExH2), meta=(BlueprintSpawnableComponent))
class EXH2_SCENEPROXY_API UExH2PrimitiveComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UExH2PrimitiveComponent();

    // ── GameThread 接口 ─────────────────────────────────────

    /**
     * CreateSceneProxy
     *
     * 在 GameThread 上被引擎调用 (RegisterComponent 触发)。
     * 返回的 FExH2SceneProxy* 由引擎通过 ENQUEUE_RENDER_COMMAND 传递给 RenderThread。
     *
     * 关键: 返回后调用方不应再通过此指针访问对象——它已属于 RenderThread。
     */
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

    // TODO [必做] 1: 调用 MarkRenderStateDirty() 观察 Proxy 重建时机
    //               (例: 在某个 UFUNCTION 里修改 DebugLabel 后调用)

    // TODO [进阶] 1: 重写 GetDynamicMeshElements 的 ViewRelevance 令其返回
    //               bDynamicRelevance=true, 再让 GetDynamicMeshElements 提交 FMeshBatch

    /** 调试标签，演示 GameThread 数据如何传递给 RenderThread Proxy */
    UPROPERTY(EditAnywhere, Category="ExH2")
    FString DebugLabel = TEXT("ExH2_Proxy");
};
