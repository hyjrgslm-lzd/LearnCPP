// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-2
// 小节: ELoadingPhase 对照观察 — 不同阶段下 UObject 系统与委托的就绪状态
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

// ══════════════════════════════════════════════════════
// FA2LoadingPhaseModule: 通过改变 .uproject 里的 LoadingPhase，
// 观察 StartupModule 调用时 UObject 系统与 FCoreDelegates 的就绪状态。
// ══════════════════════════════════════════════════════
class FA2LoadingPhaseModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // 打印当前硬编码的 phase 名称（学员需随 LoadingPhase 同步修改）
    static void PrintCurrentPhase(const TCHAR* PhaseName);

    // 探测 UObject 系统是否就绪（调用 UObject::StaticClass()）
    static void ProbeUObjectSystem();

    // TODO [必做] 2: 订阅 FCoreDelegates::OnPostEngineInit
    //   验证在早期 phase 订阅是否能在 PostEngineInit 时收到回调
    // void SubscribePostEngineInit();
};
