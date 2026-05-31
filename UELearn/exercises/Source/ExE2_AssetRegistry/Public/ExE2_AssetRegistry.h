// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-2
// 小节: IAssetRegistry 元数据查询 — GetAssetsByClass / GetDependencies / GetReferencers
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * FExE2AssetRegistryModule — 在 StartupModule 里注册 OnFilesLoaded 回调，
 * 等 AssetRegistry 扫描完毕后执行 GetAssetsByClass 查询。
 *
 * 关键坑: 不要在 StartupModule 里直接查询——AssetRegistry 扫描是异步的，
 *         此时结果可能不完整，应等 OnFilesLoaded 委托触发后再查。
 */
class FExE2AssetRegistryModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // AssetRegistry 扫描完毕后的回调
    void OnAssetRegistryFilesLoaded();

    // TODO [必做] 1: 在 OnAssetRegistryFilesLoaded 里调用 GetAssetsByClass
    //               查询 UStaticMesh，打印 Count 和第一个资产的 PackageName / TagsAndValues

    // TODO [必做] 2: 用 GetDependencies 打印硬依赖 / 软依赖数量与路径

    // TODO [必做] 3: 用 GetReferencers 打印前 5 个反向依赖

    // TODO [进阶] 1: 递归 BFS 打印完整依赖树（深度 ≤ 2）

    // 委托句柄，ShutdownModule 时解绑，防止回调野指针
    FDelegateHandle FilesLoadedHandle;
};
