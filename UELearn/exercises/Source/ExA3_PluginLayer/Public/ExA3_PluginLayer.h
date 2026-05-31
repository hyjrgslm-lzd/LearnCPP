// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-3
// 小节: 跨 Module 依赖与循环依赖打破 + Plugin 层级关系
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

// ══════════════════════════════════════════════════════
// FA3PluginLayerModule: 演示通过 IPluginManager 观察插件层加载时序，
// 并演示 PublicDependency vs PrivateDependency 的可见性差异骨架。
//
// 本模块教学重点:
//   1. Plugin 是 Module 的容器，.uplugin 的 Modules 数组描述每个 Module 的加载阶段
//   2. IPluginManager::LoadModulesForEnabledPlugins 在每个 phase 驱动 Plugin 侧加载
//   3. 用 FModuleManager::GetModuleChecked<T> 实现运行时解耦（不需要编译期依赖）
// ══════════════════════════════════════════════════════
class FA3PluginLayerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // 打印已启用插件列表与各插件的 Module 信息
    static void PrintEnabledPlugins();

    // TODO [必做] 2: 实现运行时动态加载另一个 Module 的服务
    //   static void TryGetServiceFromOtherModule();
};
