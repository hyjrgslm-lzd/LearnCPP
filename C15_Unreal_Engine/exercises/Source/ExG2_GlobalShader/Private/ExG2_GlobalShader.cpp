// ============================================================
// 对应章节: ../../../09-模块G-RHI与着色器.md §练习 G-2
// C++ 标准要求: C++20
// 本题目标: 最小 FGlobalShader + BEGIN_SHADER_PARAMETER_STRUCT + IMPLEMENT_GLOBAL_SHADER
//           完整走通 C++ 声明→.usf 编写→静态注册→RenderThread Dispatch 全链路
//
// 骨架阶段预期行为: 模块注册，StartupModule 打印 shader 路径映射信息
// 完成后预期行为（Output Log）:
//   LogExG2: Compiling global shader FExG2ComputeShader（Editor 启动时）
//   LogExG2: [RenderThread] Dispatch 完成，64 个线程各写入 index^2
// ============================================================

#include "ExG2_GlobalShader.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformTLS.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ShaderCompilerCore.h"
#include "ShaderCore.h"          // AddShaderSourceDirectoryMapping
#include "Misc/Paths.h"
#include "RenderGraphUtils.h"    // FComputeShaderUtils

// ── IMPLEMENT_GLOBAL_SHADER 必须在 .cpp，不能在 .h（ODR 规则）──────────
// 签名：IMPLEMENT_GLOBAL_SHADER(ShaderClass, SourceFilename, FunctionName, Frequency)
// 展开后是进程级静态变量，在 DLL 加载时（main() 之前）完成注册
// 参考: Engine/Source/Runtime/RenderCore/Public/GlobalShader.h 第 410 行
IMPLEMENT_GLOBAL_SHADER(FExG2ComputeShader, "/Project/ExG2.usf", "MainCS", SF_Compute);

DEFINE_LOG_CATEGORY_STATIC(LogExG2, Log, All);

class FExG2Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogExG2, Log, TEXT("[GameThread] ExG2_GlobalShader 模块启动，注册 shader 路径映射"));

        // ══════════════════════════════════════════════════════
        // Shader 路径映射：将虚拟路径 "/Project" 映射到本模块的 Shaders/ 目录
        // 这是 G2/G3 需要 Projects 模块依赖的原因（FPaths::ProjectDir）
        // AddShaderSourceDirectoryMapping 声明: ShaderCore.h 第 1643 行
        //
        // 注意：映射必须在 shader 编译之前完成（通常在模块 StartupModule 里）
        // ══════════════════════════════════════════════════════
        const FString ShaderDir = FPaths::ProjectDir() / TEXT("Source/ExG2_GlobalShader/Shaders");
        AddShaderSourceDirectoryMapping(TEXT("/Project"), ShaderDir);

        UE_LOG(LogExG2, Log, TEXT("[GameThread] Shader 路径映射: /Project -> %s"), *ShaderDir);

        // ══════════════════════════════════════════════════════
        // TODO [必做] 1: 在 ENQUEUE_RENDER_COMMAND 里完成 Dispatch：
        //   1. TShaderMapRef<FExG2ComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
        //   2. 创建 FRHIBufferCreateDesc 描述符，CreateBuffer 创建 UAV buffer
        //   3. 创建 FRHIViewDesc::CreateBufferUAV() 的 UAV 视图
        //   4. 填写 FExG2ComputeShader::FParameters
        //   5. FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Params, FIntVector(1,1,1))
        //
        // 注意：GetGlobalShaderMap 只能在 RenderThread 安全调用
        // 参考: Engine/Source/Runtime/RenderCore/Public/GlobalShader.h
        // ══════════════════════════════════════════════════════

        ENQUEUE_RENDER_COMMAND(ExG2_DispatchCompute)(
            [](FRHICommandListImmediate& RHICmdList)
            {
                UE_LOG(LogExG2, Log, TEXT("[RenderThread] ThreadId=%u，准备 Dispatch ComputeShader"),
                    FPlatformTLS::GetCurrentThreadId());

                // TODO [必做] 1: 获取编译好的 shader 实例
                // TShaderMapRef<FExG2ComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                // if (!ComputeShader.IsValid())
                // {
                //     UE_LOG(LogExG2, Warning, TEXT("[RenderThread] shader 未编译（ShouldCompilePermutation 返回 false 或路径错误）"));
                //     return;
                // }

                // TODO [必做] 2: 创建输出 UAV buffer（64 个 uint32 元素）
                // FRHIBufferCreateDesc BufDesc = FRHIBufferCreateDesc::Create(
                //     TEXT("ExG2_OutputBuffer"),
                //     64 * sizeof(uint32),
                //     sizeof(uint32),
                //     EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::ShaderResource);
                // BufDesc.SetInitialState(ERHIAccess::UAVCompute);
                // FBufferRHIRef OutputBuffer = RHICmdList.CreateBuffer(BufDesc);

                // TODO [必做] 3: 创建 UAV 视图（UE 5.6+ API）
                // FUnorderedAccessViewRHIRef OutputUAV = RHICmdList.CreateUnorderedAccessView(
                //     OutputBuffer,
                //     FRHIViewDesc::CreateBufferUAV().SetType(FRHIViewDesc::EBufferType::Typed).SetFormat(PF_R32_UINT));

                // TODO [必做] 4: 填写参数 + Dispatch
                // FExG2ComputeShader::FParameters Params;
                // Params.ThreadCount = 64;
                // Params.OutputBuffer = OutputUAV;
                // FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, Params, FIntVector(1, 1, 1));

                UE_LOG(LogExG2, Log, TEXT("[RenderThread] TODO: 完成 FExG2ComputeShader Dispatch"));
            }
        );

        // ──────────────────────────────────────────────────
        // TODO [进阶] 1: 添加 FEnableSquaredDim permutation dimension，
        //               观察 shader compile 数量翻倍
        // TODO [进阶] 2: 打开 r.DumpShaderDebugInfo=1，
        //               查看 Saved/ShaderDebugInfo/ 里自动生成的 cbuffer 声明
        // ──────────────────────────────────────────────────
    }

    virtual void ShutdownModule() override
    {
        FlushRenderingCommands();
        UE_LOG(LogExG2, Log, TEXT("[GameThread] ExG2_GlobalShader 模块卸载"));
    }
};

IMPLEMENT_MODULE(FExG2Module, ExG2_GlobalShader);

// ---- 验证区 ----
// Editor 启动后在 Output Log 搜索 "FExG2ComputeShader"，应出现 "Compiling shader" 日志
// ensureMsgf(ComputeShader.IsValid(), TEXT("shader 未能编译，检查 ShouldCompilePermutation 和路径映射"));
