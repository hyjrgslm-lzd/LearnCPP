// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-3
// 小节: 手写最小 RDG Compute Pass (implement-own)
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "GlobalShader.h"
#include "ShaderParameterMacros.h"
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"

// ============================================================
// ExH3_ComputePass 骨架
//
// 教学重点:
//   1. BEGIN_SHADER_PARAMETER_STRUCT — UE 第二处代码生成 (非 UHT)
//      展开产生 FParameters struct，字段被 RDG 反射以建立 producer/consumer 边
//   2. FExH3ComputeShader : public FGlobalShader
//      使用 DECLARE_GLOBAL_SHADER + SHADER_USE_PARAMETER_STRUCT
//   3. FComputeShaderUtils::AddPass — 一行完成 AddPass + Dispatch
//      等价于教材中 GraphBuilder.AddPass(...) + FComputeShaderUtils::Dispatch(...)
//   4. 零行 barrier 代码: 所有 transition 由 SHADER_PARAMETER_RDG_TEXTURE/UAV 宏驱动
//
// Shader 文件路径规划 (骨架阶段只声明，不提供真实 .usf):
//   虚路径 /ExH3Shaders/ExH3Compute.usf  →  实际路径由 AddShaderSourceDirectoryMapping 映射
//   在 StartupModule() 里调用:
//     FShaderSourceDirectoryMapping::AddMapping(TEXT("/ExH3Shaders"), FPaths::...);
//
// 源码校对路径:
//   Engine/Source/Runtime/RenderCore/Public/GlobalShader.h
//   Engine/Source/Runtime/RenderCore/Public/RenderGraphUtils.h    (FComputeShaderUtils)
//   Engine/Source/Runtime/RenderCore/Public/RenderGraphResources.h (FRDGTextureDesc)
//   Engine/Source/Runtime/RenderCore/Public/ShaderParameterMacros.h
// ============================================================

// ── Shader 参数 Struct (第二处代码生成) ──────────────────────

/**
 * FExH3ComputePassParameters
 *
 * BEGIN_SHADER_PARAMETER_STRUCT 展开后产生一个包含下列字段的结构体。
 * RDG 在 Execute() 的 Compile() 阶段反射这些字段，建立 producer/consumer 边：
 *   - InputTexture  → 本 pass 是该资源的 consumer (SRV 读)
 *   - OutputTexture → 本 pass 是该资源的 producer (UAV 写)
 * 从而自动推导并插入 barrier，无需手写 RHICmdList.Transition(...)
 */
BEGIN_SHADER_PARAMETER_STRUCT(FExH3ComputePassParameters, )
    // 声明 SRV 读绑定：RDG 将在 InputTexture 的 producer pass 与本 pass 之间插入 barrier
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
    // 声明 UAV 写绑定：RDG 将本 pass 记录为 OutputTexture 的 producer
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
END_SHADER_PARAMETER_STRUCT()

// ── Compute Shader 类 ─────────────────────────────────────

/**
 * FExH3ComputeShader
 *
 * 继承 FGlobalShader，与 FExH3ComputePassParameters 绑定。
 * IMPLEMENT_GLOBAL_SHADER 宏在 .cpp 中完成注册（静态初始化时执行）。
 *
 * 骨架阶段不提供真实 .usf 文件；添加 TODO 注释指导学员补全。
 *
 * 常见坑:
 *   - 虚路径必须以 "/Game/" 或自定义映射路径开头
 *   - SF_Compute 对应 compute shader 阶段，不能填 SF_Pixel
 */
class FExH3ComputeShader : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FExH3ComputeShader);
    SHADER_USE_PARAMETER_STRUCT(FExH3ComputeShader, FGlobalShader);

public:
    using FParameters = FExH3ComputePassParameters;

    // TODO [必做] 1: 在 .usf 里实现 MainCS，读取 InputTexture，写入 OutputTexture
    //               例: [numthreads(8,8,1)] void MainCS(uint3 DTid : SV_DispatchThreadID)
    //               { OutputTexture[DTid.xy] = InputTexture.Load(int3(DTid.xy, 0)); }
};

// ── 对外接口函数声明 ──────────────────────────────────────

/**
 * ExH3_AddComputePass
 *
 * 在 RenderThread 上调用，向传入的 GraphBuilder 添加一个 compute pass。
 * 骨架阶段：函数签名完整，实现在 .cpp 中留 TODO。
 *
 * @param GraphBuilder   由调用方传入的 FRDGBuilder (描述期句柄)
 * @param InputTexture   描述期句柄，某个先前 pass 已写入
 * @param OutputTexture  描述期句柄，本 pass 将写入 (需用 TexCreate_UAV 创建)
 * @param TextureSize    Dispatch 时计算 GroupCount 用
 */
void ExH3_AddComputePass(
    FRDGBuilder& GraphBuilder,
    FRDGTextureRef InputTexture,
    FRDGTextureRef OutputTexture,
    const FIntPoint& TextureSize);

// ── 模块接口 ────────────────────────────────────────────

/** ExH3 模块接口 — StartupModule 里注册 Shader 源码目录映射 */
class FExH3ComputePassModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
