// ============================================================
// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-3
// C++ 标准要求: C++20
// 本题目标: 演示 FStreamableManager::RequestAsyncLoad + FStreamableHandle 生命周期
//
// 核心类比（StreamableHandle ↔ stdexec sender）:
//   RequestAsyncLoad(Path, Lambda) 返回 Handle  ↔  构造 sender（描述工作，I/O 已入队）
//   Handle->GetLoadedAsset() 在 CompleteDelegate  ↔  sender 完成后取值
//   Handle->CancelHandle()                         ↔  stop_token::request_stop()
//   AsyncLoadHighPriority                          ↔  高优先级 scheduler
//
// 骨架阶段预期行为: BeginPlay 打印 Handle 返回后 HasLoadCompleted() 状态
//
// 完成后预期日志（Output Log 过滤 LogExE3）:
//   LogExE3: [ExE3] BeginPlay — 发起异步加载: /Game/...
//   LogExE3: [ExE3] Handle 已返回，HasLoadCompleted=0（加载尚未完成，I/O 在 AsyncLoadingThread）
//   LogExE3: [ExE3] CompleteDelegate — 加载完成，资产: Shape_Cube
// ============================================================

#include "ExE3_Streamable.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExE3, Log, All);

IMPLEMENT_MODULE(FDefaultModuleImpl, ExE3_Streamable);

// ────────────────────────────────────────────────────────────────────────────
AExE3StreamActor::AExE3StreamActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

// ────────────────────────────────────────────────────────────────────────────
// BeginPlay — 发起异步加载请求
// ────────────────────────────────────────────────────────────────────────────
void AExE3StreamActor::BeginPlay()
{
    Super::BeginPlay();

    if (AssetToLoad.IsNull())
    {
        UE_LOG(LogExE3, Warning,
            TEXT("[ExE3] AssetToLoad 路径为空，请在 Details 面板填写有效资产路径后再 PIE"));
        return;
    }

    UE_LOG(LogExE3, Log, TEXT("[ExE3] BeginPlay — 发起异步加载: %s"),
        *AssetToLoad.ToString());

    // ── 捕获 TWeakObjectPtr 防止 CompleteDelegate 触发时 Actor 已被销毁 ──
    TWeakObjectPtr<AExE3StreamActor> WeakThis(this);

    // ── RequestAsyncLoad ────────────────────────────────────────────────
    // 此调用在 GameThread 上同步返回；磁盘 I/O 在 AsyncLoadingThread 异步执行
    // bManageActiveHandle=true: Manager 内部也保住 Handle，防止 TSharedPtr 提前析构
    StreamableHandle = StreamableManager.RequestAsyncLoad(
        AssetToLoad,
        FStreamableDelegate::CreateLambda([WeakThis]()
        {
            // CompleteDelegate 在 GameThread 上被调用
            if (!WeakThis.IsValid())
            {
                return;  // Actor 已被销毁，安全退出
            }
            AExE3StreamActor* Self = WeakThis.Get();

            if (Self->StreamableHandle.IsValid() && Self->StreamableHandle->HasLoadCompleted())
            {
                UObject* LoadedObj = Self->StreamableHandle->GetLoadedAsset();
                if (LoadedObj)
                {
                    UE_LOG(LogExE3, Log,
                        TEXT("[ExE3] CompleteDelegate — 加载完成，资产: %s, 类: %s"),
                        *LoadedObj->GetName(),
                        *LoadedObj->GetClass()->GetName());
                }
                else
                {
                    UE_LOG(LogExE3, Warning,
                        TEXT("[ExE3] CompleteDelegate — HasLoadCompleted=true 但 GetLoadedAsset()=null，路径可能有误"));
                }
            }

            // ══════════════════════════════════════════════════════
            // TODO [必做] 2: 用 GetLoadedAsset<UStaticMesh>() 验证类型
            // UStaticMesh* Mesh = Self->StreamableHandle->GetLoadedAsset<UStaticMesh>();
            // ensureMsgf(Mesh != nullptr, TEXT("资产应为 UStaticMesh 类型"));
            // ══════════════════════════════════════════════════════
        }),
        FStreamableManager::DefaultAsyncLoadPriority,
        /*bManageActiveHandle=*/true,
        /*bStartStalled=*/false,
        TEXT("ExE3_AsyncLoad")
    );

    // ── 请求返回后立即查询状态（此时磁盘 I/O 尚未完成）──────────────────
    UE_LOG(LogExE3, Log,
        TEXT("[ExE3] Handle 已返回，HasLoadCompleted=%d（0=尚未完成，I/O 在 AsyncLoadingThread）"),
        StreamableHandle.IsValid() ? (int32)StreamableHandle->HasLoadCompleted() : -1);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 在下一帧 Tick（开启 bCanEverTick）或 TimerManager 里再次查询
    //              StreamableHandle->HasLoadCompleted()，观察何时变为 true
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 试验 CancelHandle
    // StreamableHandle->CancelHandle();
    // UE_LOG(LogExE3, Log, TEXT("[ExE3] CancelHandle 后 WasCanceled=%d"),
    //     (int32)StreamableHandle->WasCanceled());
    // 预期: CompleteDelegate 不被调用
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 改用 AsyncLoadHighPriority，记录从 BeginPlay 到 CompleteDelegate 的帧数差
    // StreamableHandle = StreamableManager.RequestAsyncLoad(
    //     AssetToLoad, Lambda,
    //     FStreamableManager::AsyncLoadHighPriority, ...);
    // ══════════════════════════════════════════════════════
}

// ────────────────────────────────────────────────────────────────────────────
// EndPlay — 释放 Handle（允许 GC 回收已加载资产）
// ────────────────────────────────────────────────────────────────────────────
void AExE3StreamActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (StreamableHandle.IsValid())
    {
        // ReleaseHandle: 告知 Manager 不再需要保活这批资产，下次 GC 时可回收
        StreamableHandle->ReleaseHandle();
        StreamableHandle.Reset();
        UE_LOG(LogExE3, Log, TEXT("[ExE3] EndPlay — StreamableHandle 已释放"));
    }

    Super::EndPlay(EndPlayReason);
}

// ---- 验证区 ----
// ensureMsgf(StreamableHandle.IsValid(), TEXT("RequestAsyncLoad 应返回有效 Handle"));
// ensureMsgf(!AssetToLoad.IsNull(), TEXT("AssetToLoad 路径不应为空"));
