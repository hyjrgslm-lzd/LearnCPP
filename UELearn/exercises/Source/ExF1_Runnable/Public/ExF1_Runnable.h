// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-1
// 小节: FRunnable 手动线程 + FEvent 同步
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/Event.h"

// ============================================================
// FExF1Worker — 派生自 FRunnable 的 worker 类骨架
//
// 线程生命周期：
//   Init()  ← 在新线程上调用（不是 Create 所在线程）
//   Run()   ← 主执行体，循环运行直到 bShouldStop
//   Stop()  ← 由外部（主线程）调用，设置退出标志
//   Exit()  ← Run() 返回后在新线程上清理
// ============================================================
class FExF1Worker : public FRunnable
{
public:
    // 构造时传入两个 FEventRef 引用（由调用方持有，worker 仅持有指针）
    // ReadyEvent  : Run() 开始时 Trigger，主线程在 Create 后 Wait
    // DoneEvent   : Run() 结束时 Trigger，主线程取结果后 Wait
    FExF1Worker(FEvent* InReadyEvent, FEvent* InDoneEvent);

    // ── FRunnable 接口 ─────────────────────────────────────
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    // 主线程读取计算结果（仅在 DoneEvent 被 Trigger 后安全调用）
    int64 GetResult() const { return Result; }

private:
    // 同步事件（不拥有，由 StartupModule 的 FEventRef 管理生命周期）
    FEvent* ReadyEvent  = nullptr;
    FEvent* DoneEvent   = nullptr;

    // TODO [必做] 用 FThreadSafeBool 替换裸 bool，演示原子标志
    // FThreadSafeBool bShouldStop { false };
    bool bShouldStop = false;

    // 计算结果：对 1000 万次循环计数
    int64 Result = 0;

    // worker 线程 ID（Run() 内部通过 FPlatformTLS::GetCurrentThreadId() 写入）
    uint32 WorkerThreadId = 0;
};
