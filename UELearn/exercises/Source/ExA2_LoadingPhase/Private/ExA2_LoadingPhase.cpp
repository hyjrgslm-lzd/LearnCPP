// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-2
// C++ 标准要求: C++20
// 本题目标: 通过改变 LoadingPhase，对照观察 UObject 系统与 FCoreDelegates 的就绪边界
//
// 骨架阶段预期行为: 打印 phase 名称 + UObject::StaticClass() 探测结果
// 完成后预期行为（以 Default phase 为例）:
//   LogExA2: [A2] 当前 LoadingPhase = Default
//   LogExA2: [A2] UObject::StaticClass() != nullptr: 1 (就绪)
//   LogExA2: [A2] OnPostEngineInit 订阅已注册，等待回调...
//   LogExA2: [A2] OnPostEngineInit 回调触发 (在 PostEngineInit 阶段)

#include "ExA2_LoadingPhase.h"
#include "Modules/ModuleManager.h"
#include "Misc/CoreDelegates.h"
#include "UObject/Class.h"

// ══════════════════════════════════════════════════════
// 独立日志分类
// ══════════════════════════════════════════════════════
DEFINE_LOG_CATEGORY_STATIC(LogExA2, Log, All);

IMPLEMENT_MODULE(FA2LoadingPhaseModule, ExA2_LoadingPhase);

// ══════════════════════════════════════════════════════
// 辅助：打印当前阶段名（学员按 .uproject LoadingPhase 修改此字符串）
// ══════════════════════════════════════════════════════
void FA2LoadingPhaseModule::PrintCurrentPhase(const TCHAR* PhaseName)
{
    UE_LOG(LogExA2, Log, TEXT("[A2] 当前 LoadingPhase = %s"), PhaseName);
}

// ══════════════════════════════════════════════════════
// 辅助：探测 UObject 系统就绪状态
// ══════════════════════════════════════════════════════
void FA2LoadingPhaseModule::ProbeUObjectSystem()
{
    // UObject::StaticClass() 在 UObject 系统就绪后返回非 nullptr
    // 在 PostConfigInit 阶段调用可能崩溃或返回 nullptr
    UClass* ObjClass = UObject::StaticClass();
    UE_LOG(LogExA2, Log, TEXT("[A2] UObject::StaticClass() != nullptr: %d"),
        ObjClass != nullptr ? 1 : 0);
    ensureAlways(ObjClass != nullptr);  // Default 及以后阶段应通过
}

void FA2LoadingPhaseModule::StartupModule()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 修改下面的字符串匹配 .uproject 里实际的 LoadingPhase，
    //   分别用 PostConfigInit / PreLoadingScreen / Default / PostEngineInit 启动 Editor，
    //   记录每次的 ProbeUObjectSystem 结果
    // ══════════════════════════════════════════════════════
    PrintCurrentPhase(TEXT("Default"));   // <- 学员按需修改此字符串

    ProbeUObjectSystem();

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 订阅 FCoreDelegates::OnPostEngineInit，验证回调是否触发
    //   FCoreDelegates::OnPostEngineInit.AddLambda([]()
    //   {
    //       UE_LOG(LogExA2, Log, TEXT("[A2] OnPostEngineInit 回调触发"));
    //   });
    //   UE_LOG(LogExA2, Log, TEXT("[A2] OnPostEngineInit 订阅已注册，等待回调..."));
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 制作对照表（在注释里记录实测结果）
    //   | LoadingPhase    | UObject 就绪 | OnPostEngineInit 订阅有效 |
    //   | PostConfigInit  |      ?       |            ?              |
    //   | PreLoadingScreen|      ?       |            ?              |
    //   | Default         |      ?       |            ?              |
    //   | PostEngineInit  |      ?       |            ?              |
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 阅读 ModuleManager.cpp:1078，理解 bCanProcessNewlyLoadedObjects
    //   的就绪边界与 LoadingPhase 的关系
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2: 在 DefaultEngine.ini 里加
    //   [Core.Log]
    //   LogModuleManager=Verbose
    //   观察每个 Module 加载过程的详细日志
    // ══════════════════════════════════════════════════════
}

void FA2LoadingPhaseModule::ShutdownModule()
{
    UE_LOG(LogExA2, Log, TEXT("[A2] ShutdownModule called"));
}


// ---- 验证区 (完成 TODO 后把下列行搬入对应代码路径) ----
// ensureMsgf(UObject::StaticClass() != nullptr,
//     TEXT("[A2] Default phase 时 UObject 系统应已就绪"));
// UE_LOG(LogExA2, Log, TEXT("[A2] 对照表已完成，见 TODO [必做] 3 注释"));
