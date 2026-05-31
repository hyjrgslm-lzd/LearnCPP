// ============================================================
// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-2
// C++ 标准要求: C++20
// 本题目标: GameThread Component <-> RenderThread Proxy 双对象镜像
//
// 骨架阶段预期行为 (PIE 启动后 Output Log):
//   LogExH2: [GameThread] UExH2PrimitiveComponent 已注册, 正在创建 SceneProxy
//   LogExH2: [GameThread] FExH2SceneProxy 构造, Label=ExH2_Proxy
//   LogExH2: [RenderThread] FExH2SceneProxy::GetDynamicMeshElements 被调用 (每帧)
//
// 完成后预期行为:
//   GetDynamicMeshElements 向 Collector 提交 FMeshBatch, 屏幕上可见调试几何体
// ============================================================

#include "ExH2_SceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "MeshBatch.h"
#include "MeshElementCollector.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ExH2_SceneProxy);

DEFINE_LOG_CATEGORY_STATIC(LogExH2, Log, All);

// ============================================================
// FExH2SceneProxy 实现
// (RenderThread 侧对象 — 此文件里的 Proxy 方法在 RenderThread 上执行)
// ============================================================

FExH2SceneProxy::FExH2SceneProxy(const UPrimitiveComponent* InComponent)
    // 父类构造: 按值复制 Transform / Bounds / 材质等渲染所需数据
    // 注意: 父类构造完成后, 该对象的所有权交给 FScene (RenderThread 管理)
    : FPrimitiveSceneProxy(InComponent)
{
    // 按值复制调试标签, 演示不持有 UObject*
    // 此处可以安全访问 InComponent, 因为构造在 GameThread 上发生
    // 构造完成后不再持有 InComponent 指针
    if (const UExH2PrimitiveComponent* H2Comp = Cast<UExH2PrimitiveComponent>(InComponent))
    {
        DebugLabel = H2Comp->DebugLabel;
    }

    UE_LOG(LogExH2, Log, TEXT("[GameThread] FExH2SceneProxy 构造, Label=%s"), *DebugLabel);
    UE_LOG(LogExH2, Log, TEXT("  注意: 此对象将被 ENQUEUE_RENDER_COMMAND 传递给 RenderThread"));
    UE_LOG(LogExH2, Log, TEXT("  构造后不应再从 GameThread 直接访问此对象"));
}

FExH2SceneProxy::~FExH2SceneProxy()
{
    // 析构在 RenderThread 上发生 (由 FScene::RemovePrimitive 触发)
    UE_LOG(LogExH2, Log, TEXT("[RenderThread] FExH2SceneProxy 析构, Label=%s"), *DebugLabel);
}

void FExH2SceneProxy::GetDynamicMeshElements(
    const TArray<const FSceneView*>& Views,
    const FSceneViewFamily& ViewFamily,
    uint32 VisibilityMap,
    FMeshElementCollector& Collector) const
{
    // 此函数在 RenderThread 上每帧被 FSceneRenderer 调用
    // VisibilityMap 是 bitmask，第 i 位为 1 表示 Views[i] 可见本 Primitive

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 遍历所有可见 View，向 Collector 提交 FMeshBatch
    //
    // 参考骨架:
    //   for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
    //   {
    //       if (VisibilityMap & (1 << ViewIndex))
    //       {
    //           FMeshBatch& Mesh = Collector.AllocateMesh();
    //           // 填充 Mesh.VertexFactory, Mesh.MaterialRenderProxy 等
    //           // 对于调试几何体可使用 FColoredMaterialRenderProxy
    //           Collector.AddMesh(ViewIndex, Mesh);
    //       }
    //   }
    //
    // 重要: 禁止在此函数内使用 new/delete
    //       FMeshElementCollector 提供局部 allocator，通过 AllocateMesh() 分配
    // ══════════════════════════════════════════════════════

    // 骨架阶段: 仅打印一次日志 (实际项目中此频繁路径不应有日志)
    static bool bLoggedOnce = false;
    if (!bLoggedOnce)
    {
        UE_LOG(LogExH2, Log, TEXT("[RenderThread] FExH2SceneProxy::GetDynamicMeshElements 首次调用, Label=%s"), *DebugLabel);
        bLoggedOnce = true;
    }
}

FPrimitiveViewRelevance FExH2SceneProxy::GetViewRelevance(const FSceneView* View) const
{
    FPrimitiveViewRelevance Result;

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 设置 Result.bDynamicRelevance = true
    //               使 FSceneRenderer 每帧调用 GetDynamicMeshElements
    //               骨架阶段注释掉此行, 因为骨架的 GetDynamicMeshElements 不提交 mesh
    // ══════════════════════════════════════════════════════

    // Result.bDynamicRelevance = true;  // 取消注释后 GetDynamicMeshElements 才会被调用

    return Result;
}

uint32 FExH2SceneProxy::GetMemoryFootprint() const
{
    // sizeof(*this) 加上所有动态分配内存的估算
    // 此处简单返回 sizeof(*this), 完整实现应加上 DebugLabel 的 FString 分配
    return sizeof(*this) + GetAllocatedSize();
}

SIZE_T FExH2SceneProxy::GetTypeHash() const
{
    // 标准模式: static 局部变量地址作为类唯一 TypeHash
    // 每个 FPrimitiveSceneProxy 子类都需有唯一返回值
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}

// ============================================================
// UExH2PrimitiveComponent 实现
// (GameThread 侧对象 — 由 GC 通过 UPROPERTY + Outer 链管理)
// ============================================================

UExH2PrimitiveComponent::UExH2PrimitiveComponent()
{
    // 允许 Tick (用于演示 MarkRenderStateDirty 触发时机)
    PrimaryComponentTick.bCanEverTick = true;
}

FPrimitiveSceneProxy* UExH2PrimitiveComponent::CreateSceneProxy()
{
    // 此函数在 GameThread 上调用 (由 RegisterComponent 触发的渲染状态同步)
    // 返回的指针将被 ENQUEUE_RENDER_COMMAND 传递给 RenderThread
    // 调用方保证: 返回后该对象只在 RenderThread 上使用

    UE_LOG(LogExH2, Log, TEXT("[GameThread] UExH2PrimitiveComponent::CreateSceneProxy 被调用"));
    UE_LOG(LogExH2, Log, TEXT("  此对象将由 FScene::AddPrimitive 在 RenderThread 上接管"));

    return new FExH2SceneProxy(this);
    // 返回裸指针: 不用 TSharedPtr/TUniquePtr
    // FScene 通过 FPrimitiveSceneInfo 接管所有权
}

// ══════════════════════════════════════════════════════
// TODO [必做] 3: 在某个 UFUNCTION 里修改 DebugLabel 后调用 MarkRenderStateDirty()
//               观察 CreateSceneProxy 再次被调用 (Proxy 被重建)
//   UFUNCTION(CallInEditor, Category="ExH2")
//   void RebuildProxy() { DebugLabel = TEXT("Rebuilt"); MarkRenderStateDirty(); }
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 在 FExH2SceneProxy::GetViewRelevance 里返回 bDynamicRelevance=true
//               再在 GetDynamicMeshElements 里用 Collector.AllocateMesh() 提交一个
//               wireframe cube (使用 FColoredMaterialRenderProxy)
// ══════════════════════════════════════════════════════

// ---- 验证区 (完成 TODO 后把下列断言移入对应代码路径) ----
// ensureMsgf(GIsRenderingThread || IsInRenderingThread(),
//     TEXT("GetDynamicMeshElements 必须在 RenderThread 上调用"));
// ensureMsgf(!GIsRenderingThread,
//     TEXT("CreateSceneProxy 必须在 GameThread 上调用"));
