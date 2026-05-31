// ============================================================
// 对应章节: ../../../13-结课项目1-集成项目.md
// C++ 标准要求: C++20
// 本文件: Module 入口 — IMPLEMENT_MODULE + 可选 Shader 目录映射
//
// 模块 A 知识点: IMPLEMENT_MODULE 宏展开后注册工厂函数到全局链表
// 参考: Engine/Source/Runtime/Core/Public/Modules/ModuleManager.h
//       Engine/Source/Runtime/Core/Private/Modules/ModuleManager.cpp (1061行)
// ============================================================

#include "Cap1_UHeightField.h"
#include "Modules/ModuleManager.h"

// ══════════════════════════════════════════════════════════════
// TODO [进阶] 1: 若本 Module 包含自定义 HLSL Shader, 在此添加 Shader 目录映射
// 示例:
// class FCap1UHeightFieldModule : public IModuleInterface
// {
// public:
//     virtual void StartupModule() override
//     {
//         // 将 Shaders/ 目录映射到虚拟路径 /Cap1UHeightField
//         FString ShaderDir = FPaths::Combine(FPaths::ProjectDir(),
//             TEXT("Source/Cap1_UHeightField/Shaders"));
//         AddShaderSourceDirectoryMapping(TEXT("/Cap1UHeightField"), ShaderDir);
//     }
// };
// IMPLEMENT_MODULE(FCap1UHeightFieldModule, Cap1_UHeightField);
// ══════════════════════════════════════════════════════════════

IMPLEMENT_MODULE(FDefaultModuleImpl, Cap1_UHeightField);
