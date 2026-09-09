// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-3
// 小节: 传统 RHI Draw Pass（引出 RDG 动机）
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RHICommandList.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"

// ============================================================
// 顶点着色器骨架
// ============================================================
class FExG3PassVS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FExG3PassVS);
    SHADER_USE_PARAMETER_STRUCT(FExG3PassVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        // TODO [必做] 1: 声明 ViewProjection 矩阵参数
        // SHADER_PARAMETER(FMatrix44f, ViewProjection)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

// ============================================================
// 像素着色器骨架
// 注意：RENDER_TARGET_BINDING_SLOTS() 必须放在 pixel shader 的 FParameters 中
// ============================================================
class FExG3PassPS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FExG3PassPS);
    SHADER_USE_PARAMETER_STRUCT(FExG3PassPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        // TODO [必做] 2: 声明输入 texture 和 sampler 参数
        // SHADER_PARAMETER_TEXTURE(Texture2D, InputTexture)
        // SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        // TODO [必做] 3: 声明 render target 绑定槽位
        // RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
