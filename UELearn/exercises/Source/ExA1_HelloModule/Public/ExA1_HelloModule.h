// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-1
// 小节: Hello Module — 最小可加载 Module 骨架
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

// ══════════════════════════════════════════════════════
// FA1HelloModule: 实现 IModuleInterface 的最小 Module 入口类
// 不含任何 UCLASS，不触发 UHT，只演示 Module 生命周期钩子。
// ══════════════════════════════════════════════════════
class FA1HelloModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // 辅助函数：打印 Module 启动横幅（骨架中已实现，供学员观察调用时机）
    static void PrintBootBanner();

    // TODO [必做] 进阶: 重写 SupportsDynamicReloading() 返回 false，
    // 在 Editor 内尝试 Live Coding，观察 FModuleManager::UnloadModule 行为变化
    // virtual bool SupportsDynamicReloading() override { return false; }
};
