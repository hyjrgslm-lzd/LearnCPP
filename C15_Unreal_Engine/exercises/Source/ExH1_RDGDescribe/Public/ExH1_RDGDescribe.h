// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-1
// 小节: RDG 惰性图 inspect + stdexec 类比表
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

// ============================================================
// ExH1_RDGDescribe 模块公共头
//
// 本题是纯阅读 + 分析题，核心产出是心智模型，不要求写可运行的
// RDG 工程代码。模块本体只做最小 IModuleInterface 实现。
//
// 关键教学目标:
//   1. FRDGBuilder 描述期 (AddPass 只存 lambda，不调用 GPU 命令)
//   2. FRDGBuilder::Execute() 内部三阶段:
//        Compile() → 资源分配 → pass 遍历调用 lambda
//   3. FRDGTextureRef 是描述期句柄 (FRDGTexture*)，不持有 FRHITexture
//   4. RDG ↔ stdexec 六维类比表 (见 README.md)
//
// 源码校对路径:
//   Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h
//   Engine/Source/Runtime/RenderCore/Public/RenderGraphResources.h
//   Engine/Source/Runtime/RenderCore/Public/RenderGraphDefinitions.h
// ============================================================

/** ExH1 模块接口 — 仅供 ModuleManager 注册，无业务 API */
class FExH1RDGDescribeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
