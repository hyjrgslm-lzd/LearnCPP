// 对应章节: ../../../13-结课项目1-集成项目.md §模块D: 自定义 Asset 类型
// 小节: UHeightField 继承 UObject, 作为可序列化的高度图 Asset
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UHeightFieldAsset.generated.h"

// ══════════════════════════════════════════════════════════════
// UHeightFieldAsset: 自定义 Asset 类型
// 模块 D 知识点:
//   - UCLASS() 让 UHT 产生反射元数据
//   - GENERATED_BODY() 注入反射支撑代码 (必须放第一行)
//   - UPROPERTY(EditAnywhere) 让 Editor 可编辑 + GC 可追踪
//   - 禁止裸 new UObject: 必须用 NewObject<T> 或 CreateDefaultSubobject<T>
// ══════════════════════════════════════════════════════════════

UCLASS(BlueprintType, EditInlineNew)
class CAP1_UHEIGHTFIELD_API UHeightFieldAsset : public UObject
{
    GENERATED_BODY()

public:
    UHeightFieldAsset();

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 1 (模块 D): 定义 Size 和 Resolution 属性
    // Size: 世界空间边长 (cm), Resolution: 像素分辨率 (必须是 2 的幂次)
    // ──────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeightField|Config")
    int32 Size;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeightField|Config",
        meta = (ClampMin = "4", ClampMax = "4096"))
    int32 Resolution;

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 2 (模块 B): CPU 高度图数据容器
    // TArray<uint16>: 比 float 节省一半内存; 范围 [0, 65535] 映射到 [0.0, 1.0]
    // 参考: Engine/Source/Runtime/Core/Public/Containers/Array.h
    // ──────────────────────────────────────────────────────────
    // 注意: TArray<uint16> 不是 Blueprint 可识别类型, 故不暴露给 BP (移除 BlueprintReadOnly)
    UPROPERTY(EditAnywhere, Category = "HeightField|Data")
    TArray<uint16> Heights;

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 3: 生成测试数据 (sin/cos 渐变, 便于 PNG 验证)
    // 在 PIE 内调用: HeightField->GenerateTestData(256)
    // ──────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "HeightField")
    void GenerateTestData(int32 InResolution);

    // ──────────────────────────────────────────────────────────
    // TODO [进阶] 1: 从 CSV 文件读取高度数据
    // ──────────────────────────────────────────────────────────
};
