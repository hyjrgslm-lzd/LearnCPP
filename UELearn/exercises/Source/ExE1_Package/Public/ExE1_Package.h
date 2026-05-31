// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-1
// 小节: UPackage + FArchive 双向序列化 (FMemoryWriter / FMemoryReader)
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ExE1_Package.generated.h"

/**
 * UExE1Serializable — 带三个 UPROPERTY 字段的简单 UObject，
 * 用于演示 FMemoryWriter/FMemoryReader 的往返序列化。
 *
 * 注意: UObject::Serialize() 走 Tagged Property Serialization，
 *       字段带 FName tag，可与手动写入的 raw 字节区分。
 */
UCLASS(BlueprintType)
class EXE1_PACKAGE_API UExE1Serializable : public UObject
{
    GENERATED_BODY()

public:
    UExE1Serializable();

    // ── TODO [必做] 1: 赋初值后序列化，再反序列化到 Clone，打印字段值验证一致性 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "E1")
    int32 Health = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "E1")
    FString CharName = TEXT("Hero");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "E1")
    float Speed = 600.f;

    // ── TODO [进阶] 1: 重写 Serialize，加入不带 UPROPERTY 的 InternalCounter ──
    // 观察 Tagged vs Untagged 在往返序列化时的差异
    // virtual void Serialize(FArchive& Ar) override;

private:
    // 进阶用: 不加 UPROPERTY，raw 序列化字段
    // int32 InternalCounter = 0;
};
