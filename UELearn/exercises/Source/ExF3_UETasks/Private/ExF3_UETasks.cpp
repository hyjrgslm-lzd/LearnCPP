// ============================================================
// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-3
// C++ 标准要求: C++20
// 本题目标: UE::Tasks::Launch + Then 依赖链 + FPipe 串行保护共享状态
//
// 骨架阶段预期行为: 模块注册，StartupModule 打印基本信息
// 完成后预期行为（Output Log）:
//   LogExF3: [Stage1] 加载数据，ThreadId=<W1>
//   LogExF3: [Stage2] 变换数据（平方），ThreadId=<W2>
//   LogExF3: [Stage3] 聚合（求和）= <Sum>，ThreadId=<W3>
//   LogExF3: [Pipe] SharedCounter 最终值 = 8000（无数据竞争）
// ============================================================

#include "ExF3_UETasks.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformTLS.h"
#include "Tasks/Task.h"
#include "Tasks/Pipe.h"

DEFINE_LOG_CATEGORY_STATIC(LogExF3, Log, All);

class FExF3Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogExF3, Log, TEXT("[StartupModule] 开始 UE::Tasks 演示"));

        // ══════════════════════════════════════════════════════
        // 演示 1：Then 三阶段链
        // UE::Tasks::Launch 签名（Task.h 第 266 行）：
        //   template<typename TaskBodyType>
        //   TTask<TInvokeResult_T<TaskBodyType>> Launch(
        //       const TCHAR* DebugName,
        //       TaskBodyType&& TaskBody,
        //       ETaskPriority Priority = ETaskPriority::Normal,
        //       ...)
        // Then 链：TTask<T> 本身无 .Then() 方法，需用带 Prerequisites 的 Launch 重载
        // ══════════════════════════════════════════════════════

        // 阶段 1：从硬编码"配置"加载原始数据
        UE::Tasks::TTask<TArray<int32>> Stage1 = UE::Tasks::Launch(
            TEXT("ExF3_Stage1_Load"),
            []() -> TArray<int32>
            {
                UE_LOG(LogExF3, Log, TEXT("[Stage1] 加载数据，ThreadId=%u"),
                    FPlatformTLS::GetCurrentThreadId());
                // 模拟加载：返回 [1, 2, 3, 4, 5]
                return TArray<int32>{ 1, 2, 3, 4, 5 };
            },
            UE::Tasks::ETaskPriority::Normal
        );

        // ══════════════════════════════════════════════════════
        // TODO [必做] 1: 补全阶段 2 和阶段 3，用带 Prerequisites 的 Launch 重载实现 Then 链
        //
        // 带前置条件的 Launch 签名（Task.h 第 280 行）：
        //   template<typename TaskBodyType, typename PrerequisitesCollectionType>
        //   TTask<...> Launch(
        //       const TCHAR* DebugName,
        //       TaskBodyType&& TaskBody,
        //       PrerequisitesCollectionType&& Prerequisites,
        //       ETaskPriority Priority = ...)
        //
        // 示例：
        //   UE::Tasks::TTask<TArray<int32>> Stage2 = UE::Tasks::Launch(
        //       TEXT("ExF3_Stage2_Transform"),
        //       [Stage1]() -> TArray<int32>
        //       {
        //           TArray<int32> Input = Stage1.GetResult();
        //           for (int32& V : Input) { V = V * V; }
        //           return Input;
        //       },
        //       Stage1  // 前置条件
        //   );
        // ══════════════════════════════════════════════════════

        // 等待 Stage1 完成（骨架阶段仅演示 Stage1）
        Stage1.Wait();
        UE_LOG(LogExF3, Log, TEXT("[StartupModule] Stage1 完成，数据元素数=%d"),
            Stage1.GetResult().Num());

        // ══════════════════════════════════════════════════════
        // 演示 2：FPipe 串行保护共享状态
        // 教学要点：FPipe 通过隐式前置条件实现 FIFO 串行，不是独立的串行线程
        // 坑：FPipe 析构前必须调用 WaitUntilEmpty()，否则触发 check(!HasWork())
        // ══════════════════════════════════════════════════════
        {
            UE::Tasks::FPipe Pipe(TEXT("ExF3_SharedCounterPipe"));
            int64 SharedCounter = 0;

            // ══════════════════════════════════════════════════════
            // TODO [必做] 2: 创建 8 个并发任务，每个通过 Pipe.Launch 向 SharedCounter 累加 1000 次
            //
            // 示例（单个任务）：
            //   Pipe.Launch(TEXT("ExF3_Increment"),
            //       [&SharedCounter]()
            //       {
            //           for (int32 i = 0; i < 1000; ++i) { ++SharedCounter; }
            //       });
            //
            // 目标：最终 SharedCounter == 8000，无数据竞争
            // ══════════════════════════════════════════════════════

            // 骨架：演示单次投递
            Pipe.Launch(TEXT("ExF3_Pipe_Demo"),
                [&SharedCounter]()
                {
                    UE_LOG(LogExF3, Log, TEXT("[Pipe] 串行执行，ThreadId=%u"),
                        FPlatformTLS::GetCurrentThreadId());
                    SharedCounter += 1;
                });

            // 必须在 Pipe 析构前排空（否则 check(!HasWork()) 崩溃）
            Pipe.WaitUntilEmpty();

            UE_LOG(LogExF3, Log, TEXT("[StartupModule] Pipe 演示完成，SharedCounter=%lld（骨架值=1，完成后应=8000）"),
                SharedCounter);
        } // Pipe 在此析构，此时 HasWork()==false，安全

        // ──────────────────────────────────────────────────
        // TODO [进阶] 1: 用 UE::Tasks::All 等待 4 个并发素数计算任务并汇总
        // TODO [进阶] 2: 阅读 TaskPrivate.h FTaskBase::TryLaunch 执行路径
        // ──────────────────────────────────────────────────

        UE_LOG(LogExF3, Log, TEXT("[StartupModule] ExF3_UETasks 演示完成"));
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogExF3, Log, TEXT("[GameThread] ExF3_UETasks 模块卸载"));
    }
};

IMPLEMENT_MODULE(FExF3Module, ExF3_UETasks);

// ---- 验证区 ----
// ensureMsgf(SharedCounter == 8000, TEXT("FPipe 保护的 SharedCounter 应为 8000"));
