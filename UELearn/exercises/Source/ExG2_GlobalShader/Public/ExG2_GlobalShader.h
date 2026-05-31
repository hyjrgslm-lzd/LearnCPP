// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-2
// 小节: FGlobalShader Compute 走一遍
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RHICommandList.h"

// ============================================================
// FExG2ComputeShader — 最小 compute shader 骨架
//
// 代码生成说明：
//   BEGIN_SHADER_PARAMETER_STRUCT 在 C++ 侧构建 FShaderParametersMetadata，
//   shader 编译器用该元数据在 .usf 前自动注入 cbuffer + resource binding 声明。
//   不需要也不应该在 .usf 中重复声明这些变量。
//
// IMPLEMENT_GLOBAL_SHADER 必须放在 .cpp，不能放在 .h（ODR 规则）。
// ============================================================
class FExG2ComputeShader : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FExG2ComputeShader);
    SHADER_USE_PARAMETER_STRUCT(FExG2ComputeShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        // TODO [必做] 1: 声明 uint32 ThreadCount 参数
        // SHADER_PARAMETER(uint32, ThreadCount)
        // TODO [必做] 2: 声明 RWBuffer<uint32> OutputBuffer UAV
        // SHADER_PARAMETER_UAV(RWBuffer<uint32>, OutputBuffer)
    END_SHADER_PARAMETER_STRUCT()

    // 控制哪些平台编译此 permutation
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    // TODO [进阶]: 添加 permutation dimension
    // class FEnableSquaredDim : SHADER_PERMUTATION_BOOL("ENABLE_SQUARED");
    // using FPermutationDomain = TShaderPermutationDomain<FEnableSquaredDim>;
};
