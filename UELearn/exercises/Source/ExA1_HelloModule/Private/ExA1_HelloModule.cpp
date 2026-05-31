// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-1
// C++ 标准要求: C++20
// 本题目标: 写出 UE 里最小可加载的 Module，在引擎启动日志里找到 StartupModule 打印
//
// 骨架阶段预期行为: Module 注册，PrintBootBanner 打印横幅
// 完成后预期行为（Editor 启动日志）:
//   LogExA1: [A1] === ExA1_HelloModule StartupModule ===
//   LogExA1: [A1] Module 已加载: ExA1_HelloModule
//   LogExA1: [A1] ShutdownModule called

#include "ExA1_HelloModule.h"
#include "Modules/ModuleManager.h"

// ══════════════════════════════════════════════════════
// 每题独立日志分类，避免与其他 Module 混杂
// ══════════════════════════════════════════════════════
DEFINE_LOG_CATEGORY_STATIC(LogExA1, Log, All);

IMPLEMENT_MODULE(FA1HelloModule, ExA1_HelloModule);

// ══════════════════════════════════════════════════════
// PrintBootBanner: 打印模块名横幅（骨架中已实现）
// ══════════════════════════════════════════════════════
void FA1HelloModule::PrintBootBanner()
{
    UE_LOG(LogExA1, Log, TEXT("[A1] === ExA1_HelloModule StartupModule ==="));
    UE_LOG(LogExA1, Log, TEXT("[A1] Module 已加载: ExA1_HelloModule"));
}

void FA1HelloModule::StartupModule()
{
    PrintBootBanner();

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 用 FModuleManager::Get().IsModuleLoaded 验证本模块已加载
    //   bool bLoaded = FModuleManager::Get().IsModuleLoaded(TEXT("ExA1_HelloModule"));
    //   UE_LOG(LogExA1, Log, TEXT("[A1] IsModuleLoaded = %d"), bLoaded);
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 加上 #if WITH_EDITOR 守卫，打印 Editor-only 日志
    //   #if WITH_EDITOR
    //       UE_LOG(LogExA1, Log, TEXT("[A1] Running in Editor build"));
    //   #endif
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 在 Public/ 下新建 IA1Service.h 声明纯接口
    //   class IA1Service { public: virtual ~IA1Service() = default; virtual void DoWork() = 0; };
    //   在 Private/ 下新建 A1ServiceImpl.h/.cpp 实现，
    //   从另一个 Module 尝试 include Private 头，观察 UBT include path 报错
    // ══════════════════════════════════════════════════════
}

void FA1HelloModule::ShutdownModule()
{
    UE_LOG(LogExA1, Log, TEXT("[A1] ShutdownModule called"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 在 ShutdownModule 里清理 StartupModule 中注册的委托或资源
    // ══════════════════════════════════════════════════════
}


// ---- 验证区 (完成 TODO 后把下列行搬入对应代码路径) ----
// ensureMsgf(FModuleManager::Get().IsModuleLoaded(TEXT("ExA1_HelloModule")),
//     TEXT("ExA1_HelloModule 未被正确加载"));
// UE_LOG(LogExA1, Log, TEXT("[A1] 验证: IMPLEMENT_MODULE 第二参数 = ExA1_HelloModule"));
