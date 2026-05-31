// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-3
// 小节: UE::Tasks 现代 API
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "Tasks/Task.h"
#include "Tasks/Pipe.h"

// ============================================================
// ExF3 模块公共头
//
// 本题核心 API（全部在 Core/Tasks/）：
//   UE::Tasks::Launch(DebugName, lambda, Priority)  → TTask<T>
//   UE::Tasks::Launch(DebugName, lambda, prereq)    → Then 等价形式
//   UE::Tasks::FPipe::Launch(DebugName, lambda)     → 串行管道
//   TTask<T>::Wait()                                → 阻塞等待
//
// 主要逻辑在 Private/ExF3_UETasks.cpp 的 StartupModule。
// ============================================================
