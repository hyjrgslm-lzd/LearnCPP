// ============================================================
// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-2
// C++ 标准要求: C++20
// 本题目标: 通过 IAssetRegistry 查询资产元数据，不触发任何资产加载
//
// 关键坑: StartupModule 里不要直接查询——AssetRegistry 扫描是异步的。
//         必须注册 OnFilesLoaded 委托，等扫描完毕后在回调里查询。
//
// 骨架阶段预期行为: 模块注册，OnFilesLoaded 委托绑定
//
// 完成后预期日志（Output Log 过滤 LogExE2）:
//   LogExE2: [ExE2] StartupModule — 已绑定 OnFilesLoaded 委托
//   LogExE2: [ExE2] OnFilesLoaded — 开始查询 UStaticMesh 资产
//   LogExE2: [ExE2] 找到 N 个 StaticMesh 资产
//   LogExE2: [ExE2] 第一个资产 PackageName: /Game/...
//   LogExE2: [ExE2] 硬依赖数量: N，软依赖数量: M
// ============================================================

#include "ExE2_AssetRegistry.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExE2, Log, All);

IMPLEMENT_MODULE(FExE2AssetRegistryModule, ExE2_AssetRegistry);

// ────────────────────────────────────────────────────────────────────────────
// StartupModule — 只注册回调，不直接查询
// ────────────────────────────────────────────────────────────────────────────
void FExE2AssetRegistryModule::StartupModule()
{
    // IAssetRegistry::Get() 在非常早期可能为 nullptr，用 IsModuleLoaded 先确认
    if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
    {
        IAssetRegistry& AR = IAssetRegistry::GetChecked();

        if (AR.IsLoadingAssets())
        {
            // 扫描未完成：注册 OnFilesLoaded 委托，等完成后再查询
            FilesLoadedHandle = AR.OnFilesLoaded().AddRaw(
                this, &FExE2AssetRegistryModule::OnAssetRegistryFilesLoaded);
            UE_LOG(LogExE2, Log, TEXT("[ExE2] StartupModule — AssetRegistry 仍在扫描，已绑定 OnFilesLoaded 委托"));
        }
        else
        {
            // 扫描已完成（例如 Editor 已完整启动后才加载本模块）
            UE_LOG(LogExE2, Log, TEXT("[ExE2] StartupModule — AssetRegistry 已就绪，直接查询"));
            OnAssetRegistryFilesLoaded();
        }
    }
    else
    {
        UE_LOG(LogExE2, Warning, TEXT("[ExE2] StartupModule — AssetRegistry 模块尚未加载，跳过"));
    }
}

// ────────────────────────────────────────────────────────────────────────────
// ShutdownModule — 解绑委托，防止野指针回调
// ────────────────────────────────────────────────────────────────────────────
void FExE2AssetRegistryModule::ShutdownModule()
{
    if (FilesLoadedHandle.IsValid())
    {
        if (IAssetRegistry* AR = IAssetRegistry::Get())
        {
            AR->OnFilesLoaded().Remove(FilesLoadedHandle);
        }
        FilesLoadedHandle.Reset();
    }
}

// ────────────────────────────────────────────────────────────────────────────
// OnAssetRegistryFilesLoaded — 扫描完毕后的查询入口
// ────────────────────────────────────────────────────────────────────────────
void FExE2AssetRegistryModule::OnAssetRegistryFilesLoaded()
{
    IAssetRegistry& AR = IAssetRegistry::GetChecked();

    // ── TODO [必做] 1: 查询 UStaticMesh 资产 ─────────────────────────────
    // UE5 中类路径格式: FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("StaticMesh"))
    TArray<FAssetData> AssetList;
    AR.GetAssetsByClass(
        FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("StaticMesh")),
        AssetList,
        /*bSearchSubClasses=*/false
    );

    UE_LOG(LogExE2, Log, TEXT("[ExE2] OnFilesLoaded — 找到 %d 个 StaticMesh 资产"), AssetList.Num());

    if (AssetList.Num() == 0)
    {
        UE_LOG(LogExE2, Warning,
            TEXT("[ExE2] 未找到 StaticMesh 资产。请在 Editor 里新建并保存一个 StaticMesh，再重试。"));
        return;
    }

    // ── 打印第一个资产的元数据 ────────────────────────────────────────────
    const FAssetData& AssetData = AssetList[0];
    UE_LOG(LogExE2, Log, TEXT("[ExE2] 第一个资产 PackageName: %s"), *AssetData.PackageName.ToString());
    UE_LOG(LogExE2, Log, TEXT("[ExE2] AssetName: %s"),      *AssetData.AssetName.ToString());
    UE_LOG(LogExE2, Log, TEXT("[ExE2] AssetClassPath: %s"), *AssetData.AssetClassPath.ToString());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 打印 TagsAndValues（每个 Tag 的 Key=Value）
    // AssetData.TagsAndValues.ForEach([](const TPair<FName, FAssetTagValueRef>& Pair)
    // {
    //     UE_LOG(LogExE2, Log, TEXT("[ExE2]   Tag: %s = %s"),
    //         *Pair.Key.ToString(), *Pair.Value.AsString());
    // });
    // ══════════════════════════════════════════════════════

    // ── TODO [必做] 3: 查询硬依赖 / 软依赖 ───────────────────────────────
    // 参考: UE::AssetRegistry::EDependencyCategory::Package
    //       UE::AssetRegistry::EDependencyProperty::Hard / Soft
    // TArray<FName> HardDeps, SoftDeps;
    // AR.GetDependencies(AssetData.PackageName, HardDeps,
    //     UE::AssetRegistry::EDependencyCategory::Package,
    //     UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyProperty::Hard));
    // AR.GetDependencies(AssetData.PackageName, SoftDeps,
    //     UE::AssetRegistry::EDependencyCategory::Package,
    //     UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyProperty::Soft));
    // UE_LOG(LogExE2, Log, TEXT("[ExE2] 硬依赖: %d，软依赖: %d"), HardDeps.Num(), SoftDeps.Num());

    // ── TODO [必做] 4: 查询反向引用 (GetReferencers)，打印前 5 个 ─────────
    // TArray<FName> Referencers;
    // AR.GetReferencers(AssetData.PackageName, Referencers);
    // int32 PrintCount = FMath::Min(Referencers.Num(), 5);
    // for (int32 i = 0; i < PrintCount; ++i)
    // {
    //     UE_LOG(LogExE2, Log, TEXT("[ExE2] 引用者[%d]: %s"), i, *Referencers[i].ToString());
    // }
}

// ---- 验证区 ----
// ensureMsgf(!AR.IsLoadingAssets(), TEXT("OnFilesLoaded 回调时扫描应已完成"));
// ensureMsgf(AssetList.Num() > 0, TEXT("应至少找到一个 StaticMesh（需先在 Editor 中创建）"));
