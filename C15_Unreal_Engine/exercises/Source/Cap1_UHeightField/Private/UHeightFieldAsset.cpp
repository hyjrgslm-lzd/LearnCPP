// ============================================================
// 对应章节: ../../../13-结课项目1-集成项目.md §模块D
// C++ 标准要求: C++20
// 本文件: UHeightFieldAsset 实现
//
// 模块 D 知识点:
//   - UObject 构造函数: 用于设置默认值 (CDO 也会经过此构造)
//   - HasAnyFlags(RF_ClassDefaultObject): 区分 CDO 与真实实例
//   - GENERATED_BODY(): 展开后注入反射支撑代码
// ============================================================

#include "UHeightFieldAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogCap1Asset, Log, All);

// ------------------------------------------------------------
// 构造函数: CDO 和实例都会经过此路径
// 注意: 不要在构造函数里调用 NewObject<T> (GC 未就绪)
// ------------------------------------------------------------
UHeightFieldAsset::UHeightFieldAsset()
{
    Size = 512;
    Resolution = 256;

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1 (模块 D): 区分 CDO 与实例
    // CDO 构造时 HasAnyFlags(RF_ClassDefaultObject) 为 true
    // 实例构造时为 false (此时才适合做实际初始化)
    // ══════════════════════════════════════════════════════
    UE_LOG(LogCap1Asset, Log, TEXT("[Cap1] UHeightFieldAsset constructed: %s (IsCDO=%d)"),
        *GetNameSafe(this),
        (int32)HasAnyFlags(RF_ClassDefaultObject));
}

// ------------------------------------------------------------
// GenerateTestData: 用 sin/cos 生成测试高度图数据
// 便于验证 pass1_heightfield.png 是否为平滑渐变
// ------------------------------------------------------------
void UHeightFieldAsset::GenerateTestData(int32 InResolution)
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 2 (模块 B): 使用 TArray 填充 CPU 高度数据
    // ══════════════════════════════════════════════════════
    Resolution = FMath::Max(4, InResolution);
    Heights.SetNumUninitialized(Resolution * Resolution);

    for (int32 Y = 0; Y < Resolution; ++Y)
    {
        for (int32 X = 0; X < Resolution; ++X)
        {
            // sin/cos 渐变: 范围 [-1, 1] → 映射到 [0, 65535]
            float NX = (float)X / (float)(Resolution - 1);
            float NY = (float)Y / (float)(Resolution - 1);
            float Height = 0.5f * (FMath::Sin(NX * 2.f * PI) + FMath::Cos(NY * 2.f * PI)) * 0.5f + 0.5f;
            Heights[Y * Resolution + X] = (uint16)FMath::Clamp(
                FMath::RoundToInt(Height * 65535.f), 0, 65535);
        }
    }

    UE_LOG(LogCap1Asset, Log,
        TEXT("[Cap1][Step 1] GameThread: UHeightField asset created (size=%d, res=%d)"),
        Size, Resolution);

    // ---- 验证区 ----
    // ensureMsgf(Heights.Num() == Resolution * Resolution,
    //     TEXT("Heights 数组大小应等于 Resolution^2"));
}

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 从 CSV 文件读取高度数据
// 使用 FFileHelper::LoadFileToString + TArray<FString> 解析
// ══════════════════════════════════════════════════════
