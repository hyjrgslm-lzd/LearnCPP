// 对应章节: ../../../13-结课项目1-集成项目.md §模块K: Actor 承载可视化
// 小节: AHeightFieldViewer — AActor 子类, 驱动异步加载 + GPU 上传 + RDG 渲染
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
// TObjectPtr: UE5 推荐包装方式, Editor 构建下追踪不安全访问
// 参考: Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h
#include "Engine/StreamableManager.h"
#include "AHeightFieldViewer.generated.h"

class UHeightFieldAsset;

// ══════════════════════════════════════════════════════════════
// AHeightFieldViewer: 模块 K (Actor) + 模块 E (StreamableManager)
//                   + 模块 F (ENQUEUE_RENDER_COMMAND) 的集成点
// 约束 (来自 §3.3):
//   - 必须继承 AActor
//   - 必须实现 BeginPlay / EndPlay
//   - 成员 UObject* 必须用 TObjectPtr<T> + UPROPERTY (禁止裸 UObject* 成员)
// ══════════════════════════════════════════════════════════════

UCLASS(BlueprintType)
class CAP1_UHEIGHTFIELD_API AHeightFieldViewer : public AActor
{
    GENERATED_BODY()

public:
    AHeightFieldViewer();

protected:
    // ──────────────────────────────────────────────────────────
    // 生命周期钩子 (模块 K 知识点)
    // BeginPlay: 注册渲染回调, 触发异步加载
    // EndPlay:   注销回调, 释放 RHI 资源
    // ──────────────────────────────────────────────────────────
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // ──────────────────────────────────────────────────────────
    // TODO [必做] 1 (模块 D/E): 软引用 + UPROPERTY GC 可见性
    // TSoftObjectPtr: 异步加载的延迟引用, 不阻塞 GameThread
    // ──────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeightField")
    TSoftObjectPtr<UHeightFieldAsset> HeightFieldAssetRef;

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 2 (模块 D/GC): 加载完成后持有 Asset 的强引用
    // TObjectPtr + UPROPERTY 保证 GC 不回收 (约束 §3.2: 禁止裸 UObject* 成员)
    // 参考: 01-心智模型.md §GC 可见性 — UPROPERTY + TObjectPtr
    // ──────────────────────────────────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "HeightField")
    TObjectPtr<UHeightFieldAsset> HeightFieldAsset;

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 3 (模块 E): 异步加载入口
    // 在 BeginPlay 里调用, 传入 FSoftObjectPath 触发 FStreamableManager
    // ──────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "HeightField")
    void StartAsyncLoad(const FSoftObjectPath& AssetPath);

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 4 (模块 F/G/H): 加载完成后触发 GPU 管线
    // 由 FStreamableManager 回调调用 (GameThread)
    // 流程: 创建 UE::Tasks worker → ENQUEUE_RENDER_COMMAND → RDG 三 pass
    // ──────────────────────────────────────────────────────────
    void OnAssetLoaded();

private:
    // ──────────────────────────────────────────────────────────
    // FStreamableHandle 持有加载句柄
    // 约束: 必须保存 handle, 否则 StreamableManager 认为请求被取消
    // 参考: 13-结课项目1 §七 坑6: FStreamableHandle 生命周期过短
    // ──────────────────────────────────────────────────────────
    TSharedPtr<FStreamableHandle> LoadHandle;

    // RHI 纹理句柄 (RenderThread 侧创建, GameThread 侧不能直接访问)
    // 使用 TRefCountPtr 而非裸指针 (非 UObject, 不走 GC)
    // TRefCountPtr<FRHITexture> HeightFieldTexture; // TODO [必做] 5 (模块 G)
};
