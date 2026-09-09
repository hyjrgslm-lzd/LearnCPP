// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-2
// 小节: TaskGraph + Named Threads
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "Async/TaskGraphInterfaces.h"

// ============================================================
// ExF2 模块公共头
//
// 本题核心 API（全部在 Core/Async/TaskGraphInterfaces.h）：
//   FFunctionGraphTask::CreateAndDispatchWhenReady(lambda, TStatId{}, prereqs, thread)
//   FTaskGraphInterface::Get().WaitUntilTaskCompletes(event, ENamedThreads::GameThread)
//   ENamedThreads::AnyNormalThreadNormalTask
//   ENamedThreads::AnyHiPriThreadNormalTask
//   ENamedThreads::GameThread
//   ENamedThreads::ActualRenderingThread
//
// 主要逻辑在 Private/ExF2_TaskGraph.cpp 的 StartupModule。
// ============================================================
