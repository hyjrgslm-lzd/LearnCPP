// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-1
// 小节: RHI 资源与 CommandList 基础
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "RHIResources.h"

// ============================================================
// ExG1 模块公共头
//
// 本题核心 API：
//   FRHIBufferCreateDesc::CreateVertex(name, size)    → 构建 buffer 描述符
//   RHICmdList.CreateBuffer(desc)                     → 创建 vertex buffer（UE 5.6+）
//   FBufferRHIRef                                     → TRefCountPtr<FRHIBuffer>
//   FTextureRHIRef                                    → TRefCountPtr<FRHITexture>
//   RHICmdList.CreateTexture(desc)                    → 创建 2D texture
//
// 注意：FRHIResourceCreateInfo 在 UE 5.6 已被标记 deprecated，
//       本题使用 FRHIBufferCreateDesc / FRHITextureCreateDesc 现代 API。
//
// 主要逻辑在 Private/ExG1_RHIResource.cpp 的 StartupModule。
// ============================================================
