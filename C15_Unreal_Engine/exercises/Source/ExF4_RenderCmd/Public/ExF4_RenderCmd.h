// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-4
// 小节: ENQUEUE_RENDER_COMMAND 跨线程（通往 G 的钥匙）
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "RenderingThread.h"
#include "RHICommandList.h"

// ============================================================
// ExF4 模块公共头
//
// 教学核心：render command lambda 必须按值捕获所有数据
//   正确：[ValueCopy](FRHICommandListImmediate& RHICmdList){ ... }
//   错误：[&Ref](FRHICommandListImmediate& RHICmdList){ ... }  ← 悬垂引用
//
// 本题 API（RenderCore/RenderingThread.h）：
//   ENQUEUE_RENDER_COMMAND(TypeName)(lambda)
//   FlushRenderingCommands()   ← 仅用于对比实验，生产代码不在 Tick 里调用
//
// 主要逻辑在 Private/ExF4_RenderCmd.cpp 的 StartupModule。
// ============================================================
