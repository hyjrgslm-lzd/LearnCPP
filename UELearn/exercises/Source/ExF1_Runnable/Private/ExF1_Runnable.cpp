// ============================================================
// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-1
// C++ 标准要求: C++20
// 本题目标: 派生 FRunnable，用 FRunnableThread::Create 启动手工管理线程，
//           用 FEventRef（RAII）做双向握手，对比 GameThread / WorkerThread ID
//
// 骨架阶段预期行为: 模块注册，StartupModule 打印 GameThread ID
// 完成后预期行为（Output Log）:
//   LogExF1: [GameThread] ThreadId=<GT_ID>，即将创建 worker 线程
//   LogExF1: [WorkerThread] Init 调用，ThreadId=<WT_ID>
//   LogExF1: [WorkerThread] Run 开始，计数 10000000 次
//   LogExF1: [WorkerThread] Run 结束，触发 DoneEvent
//   LogExF1: [GameThread] 结果 = 10000000
// ============================================================

#include "ExF1_Runnable.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformTLS.h"
#include "HAL/RunnableThread.h"
#include "HAL/Event.h"

DEFINE_LOG_CATEGORY_STATIC(LogExF1, Log, All);

// ── FExF1Worker 实现 ──────────────────────────────────────────

FExF1Worker::FExF1Worker(FEvent* InReadyEvent, FEvent* InDoneEvent)
    : ReadyEvent(InReadyEvent)
    , DoneEvent(InDoneEvent)
{
}

bool FExF1Worker::Init()
{
    // Init() 在新线程上调用，不是在 FRunnableThread::Create 所在线程
    WorkerThreadId = FPlatformTLS::GetCurrentThreadId();
    UE_LOG(LogExF1, Log, TEXT("[WorkerThread] Init 调用，ThreadId=%u"), WorkerThreadId);
    return true; // 返回 false 则 Run() 不会被调用
}

uint32 FExF1Worker::Run()
{
    // ── 握手 1：通知主线程"子线程已开始运行" ──────────────
    if (ReadyEvent)
    {
        ReadyEvent->Trigger();
    }

    UE_LOG(LogExF1, Log, TEXT("[WorkerThread] Run 开始，ThreadId=%u，准备计数 10000000 次"),
        FPlatformTLS::GetCurrentThreadId());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 将 bShouldStop 改为 FThreadSafeBool，
    //               每次循环检查 bShouldStop，实现提前退出
    //   #include "HAL/ThreadSafeBool.h"
    //   FThreadSafeBool bShouldStop { false };
    // ══════════════════════════════════════════════════════
    const int64 LoopCount = 10000000LL;
    for (int64 i = 0; i < LoopCount; ++i)
    {
        if (bShouldStop)
        {
            break;
        }
        Result = i + 1;
    }

    UE_LOG(LogExF1, Log, TEXT("[WorkerThread] Run 结束，Result=%lld，触发 DoneEvent"), Result);

    // ── 握手 2：通知主线程"计算已完成" ───────────────────
    if (DoneEvent)
    {
        DoneEvent->Trigger();
    }

    return 0;
}

void FExF1Worker::Stop()
{
    // Stop() 由外部线程（主线程）调用，设置退出标志
    // 注意：Stop() 不强制终止线程，Run() 必须主动检查 bShouldStop
    UE_LOG(LogExF1, Log, TEXT("[外部] Stop() 被调用，设置 bShouldStop=true"));
    bShouldStop = true;
}

void FExF1Worker::Exit()
{
    // Exit() 在 Run() 返回后、线程退出前，在新线程上调用
    UE_LOG(LogExF1, Log, TEXT("[WorkerThread] Exit 调用，ThreadId=%u"),
        FPlatformTLS::GetCurrentThreadId());
}

// ── 模块主类 ──────────────────────────────────────────────────

class FExF1Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        const uint32 GameThreadId = FPlatformTLS::GetCurrentThreadId();
        UE_LOG(LogExF1, Log, TEXT("[GameThread] ThreadId=%u，即将创建 worker 线程"), GameThreadId);

        // 使用 FEventRef RAII：构造时 GetSynchEventFromPool，析构时 ReturnSynchEventToPool
        // 参考: Engine/Source/Runtime/Core/Public/HAL/Event.h 第 129 行
        FEventRef ReadyEvent { EEventMode::AutoReset };
        FEventRef DoneEvent  { EEventMode::AutoReset };

        // 创建 worker（worker 仅持有裸指针，FEventRef 拥有生命周期）
        FExF1Worker* Worker = new FExF1Worker(ReadyEvent.Get(), DoneEvent.Get());

        // ══════════════════════════════════════════════════════
        // FRunnableThread::Create 签名（RunnableThread.h 第 44 行）：
        //   static FRunnableThread* Create(
        //       FRunnable* InRunnable,
        //       const TCHAR* ThreadName,
        //       uint32 InStackSize = 0,
        //       EThreadPriority InThreadPri = TPri_Normal,
        //       uint64 InThreadAffinityMask = ...,
        //       EThreadCreateFlags InCreateFlags = None)
        // ══════════════════════════════════════════════════════
        FRunnableThread* Thread = FRunnableThread::Create(
            Worker,
            TEXT("ExF1_WorkerThread"),
            0,              // 默认栈大小
            TPri_Normal
        );

        if (!Thread)
        {
            UE_LOG(LogExF1, Error, TEXT("[GameThread] FRunnableThread::Create 失败"));
            delete Worker;
            return;
        }

        // ── 握手 1：等待子线程"已开始运行"信号 ───────────
        UE_LOG(LogExF1, Log, TEXT("[GameThread] 等待 ReadyEvent（子线程启动）..."));
        ReadyEvent->Wait(); // AutoReset 模式：被 Trigger 后自动复位

        UE_LOG(LogExF1, Log, TEXT("[GameThread] 子线程已启动，继续执行主线程逻辑"));

        // ── 握手 2：等待子线程"计算完成"信号 ─────────────
        UE_LOG(LogExF1, Log, TEXT("[GameThread] 等待 DoneEvent（计算完成）..."));
        DoneEvent->Wait();

        const int64 FinalResult = Worker->GetResult();
        UE_LOG(LogExF1, Log, TEXT("[GameThread] Worker 计算结果 = %lld"), FinalResult);

        // ── 正确销毁顺序：先 Kill(true)，再 delete ────────
        // 坑：颠倒顺序（先 delete 再 Kill）= UAF（Use After Free）
        Thread->Kill(true); // true = 等待线程退出后返回
        delete Thread;
        delete Worker;

        UE_LOG(LogExF1, Log, TEXT("[GameThread] Worker 线程已销毁，GameThreadId=%u"),
            GameThreadId);

        // ──────────────────────────────────────────────────
        // TODO [进阶] 1: 用 FThreadSafeCounter 记录迭代进度，
        //               主线程在 DoneEvent.Wait() 前轮询进度
        // TODO [进阶] 2: 写等价的 std::thread + std::atomic<bool> 版本，
        //               对比 FRunnable vs std::thread，FEvent vs condition_variable
        // ──────────────────────────────────────────────────
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogExF1, Log, TEXT("[GameThread] ExF1_Runnable 模块卸载"));
    }
};

// 每个 ExXN 模块用 FDefaultModuleImpl 或自定义类均可，这里用自定义类驱动 StartupModule
IMPLEMENT_MODULE(FExF1Module, ExF1_Runnable);

// ---- 验证区（完成 TODO 后把下列 ensure 搬入对应代码路径）----
// ensureMsgf(FinalResult == 10000000LL, TEXT("计数结果应为 10000000"));
// ensureMsgf(GameThreadId != Worker->WorkerThreadId, TEXT("两个 ThreadId 应该不同"));
