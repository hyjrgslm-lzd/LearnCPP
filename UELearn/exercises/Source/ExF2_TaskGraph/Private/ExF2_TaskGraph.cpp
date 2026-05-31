// ============================================================
// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-2
// C++ 标准要求: C++20
// 本题目标: FFunctionGraphTask::CreateAndDispatchWhenReady 建立 A→B→C 依赖链，
//           演示 ENamedThreads 各线程槽位，用 WaitUntilTaskCompletes 等待
//
// 骨架阶段预期行为: 模块注册，StartupModule 打印 GameThread ID
// 完成后预期行为（Output Log）:
//   LogExF2: [TaskA] 在 AnyNormalThread 执行，ThreadId=<W1>
//   LogExF2: [TaskB] 在 AnyHiPriThread 执行，ThreadId=<W2>（依赖 A 完成）
//   LogExF2: [TaskC] 在 GameThread 执行，ThreadId=<GT>（依赖 B 完成）
// ============================================================

#include "ExF2_TaskGraph.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformTLS.h"
#include "Async/TaskGraphInterfaces.h"

DEFINE_LOG_CATEGORY_STATIC(LogExF2, Log, All);

class FExF2Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        const uint32 GameTid = FPlatformTLS::GetCurrentThreadId();
        UE_LOG(LogExF2, Log, TEXT("[StartupModule] GameThread ThreadId=%u"), GameTid);

        // ══════════════════════════════════════════════════════
        // TODO [必做] 1: 创建任务 A（无前置，投递到 AnyNormalThread）
        // 签名：FFunctionGraphTask::CreateAndDispatchWhenReady(
        //           TUniqueFunction<void()>,   // lambda
        //           TStatId{},                 // stat id（调试用，可传 {}）
        //           nullptr,                   // 无前置
        //           ENamedThreads::AnyNormalThreadNormalTask)
        // 参考: Engine/Source/Runtime/Core/Public/Async/TaskGraphInterfaces.h 第 1128 行
        // ══════════════════════════════════════════════════════
        FGraphEventRef EventA = FFunctionGraphTask::CreateAndDispatchWhenReady(
            []()
            {
                UE_LOG(LogExF2, Log,
                    TEXT("[TaskA] AnyNormalThread 执行，ThreadId=%u，NamedThread=%d"),
                    FPlatformTLS::GetCurrentThreadId(),
                    (int32)FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
            },
            TStatId{},
            nullptr,
            ENamedThreads::AnyNormalThreadNormalTask
        );

        // ══════════════════════════════════════════════════════
        // TODO [必做] 2: 创建任务 B（前置 = EventA，投递到 AnyHiPriThread）
        // 用 FGraphEventArray Prerequisites; Prerequisites.Add(EventA); 传入
        // ══════════════════════════════════════════════════════
        FGraphEventRef EventB = FFunctionGraphTask::CreateAndDispatchWhenReady(
            []()
            {
                UE_LOG(LogExF2, Log,
                    TEXT("[TaskB] AnyHiPriThread 执行，ThreadId=%u，NamedThread=%d"),
                    FPlatformTLS::GetCurrentThreadId(),
                    (int32)FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
            },
            TStatId{},
            EventA,   // 单个前置重载（TaskGraphInterfaces.h 第 1146 行）
            ENamedThreads::AnyHiPriThreadNormalTask
        );

        // ══════════════════════════════════════════════════════
        // TODO [必做] 3: 创建任务 C（前置 = EventB，投递到 GameThread）
        // 注意：投递到 GameThread 的任务只有在 GameThread 消耗队列时才执行
        //       WaitUntilTaskCompletes 内部会驱动 GameThread 排空本地队列
        // ══════════════════════════════════════════════════════
        FGraphEventRef EventC = FFunctionGraphTask::CreateAndDispatchWhenReady(
            [GameTid]()
            {
                const uint32 CurTid = FPlatformTLS::GetCurrentThreadId();
                UE_LOG(LogExF2, Log,
                    TEXT("[TaskC] GameThread 执行，ThreadId=%u（应与 GameTid=%u 相同）"),
                    CurTid, GameTid);
            },
            TStatId{},
            EventB,
            ENamedThreads::GameThread
        );

        // ── 等待链尾 C 完成（在 GameThread 上等待，内部会处理 GameThread 队列）
        UE_LOG(LogExF2, Log, TEXT("[GameThread] 开始等待 TaskC 完成..."));
        FTaskGraphInterface::Get().WaitUntilTaskCompletes(EventC, ENamedThreads::GameThread);
        UE_LOG(LogExF2, Log, TEXT("[GameThread] TaskC 已完成，A→B→C 依赖链验证通过"));

        // ══════════════════════════════════════════════════════
        // TODO [必做] 4: 额外创建一个投递到 ActualRenderingThread 的任务，
        //               打印其 ThreadId，与 GameThread ThreadId 对比
        // ══════════════════════════════════════════════════════

        // ──────────────────────────────────────────────────
        // TODO [进阶] 1: 用 TGraphTask<T> 完整路径（不用 FFunctionGraphTask）
        //               实现 GetDesiredThread() + DoTask() 自定义任务类型
        // TODO [进阶] 2: 实验 FGraphEventRef::DontCompleteUntil(other)
        //               延长任务完成时刻，体会与"作为前置条件"的语义差异
        // ──────────────────────────────────────────────────
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogExF2, Log, TEXT("[GameThread] ExF2_TaskGraph 模块卸载"));
    }
};

IMPLEMENT_MODULE(FExF2Module, ExF2_TaskGraph);

// ---- 验证区 ----
// 完成 TODO 后取消注释（在 WaitUntilTaskCompletes 返回后执行）:
// ensureMsgf(EventA->IsComplete(), TEXT("TaskA 应已完成"));
// ensureMsgf(EventB->IsComplete(), TEXT("TaskB 应已完成"));
// ensureMsgf(EventC->IsComplete(), TEXT("TaskC 应已完成"));
