// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-1
// 小节: 最小 UCLASS + GENERATED_BODY() 展开 + CDO 构造时机
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
// 注意: .generated.h 必须是头文件的最后一个 include (UHT 硬性要求)
// 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h:765
#include "ExD1_HelloUObject.generated.h"

// ══════════════════════════════════════════════════════
// 最小 UCLASS 示例
// GENERATED_BODY() 展开三块:
//   1. INCLASS_NO_PURE_DECLS  —— StaticClass() / GetPrivateStaticClass() 声明
//   2. ENHANCED_CONSTRUCTORS  —— 删除的拷贝/移动构造, CDO 初始化入口
//   3. CALLBACK_WRAPPERS      —— BlueprintCallable 函数的蓝图桥接声明
// 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h:764-768
// ══════════════════════════════════════════════════════
UCLASS(BlueprintType, Blueprintable)
class EXD1_HELLOUOBJECT_API UExD1SampleObject : public UObject
{
    GENERATED_BODY()

public:
    UExD1SampleObject();

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 声明 UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Counter
    //   初值 0; 在构造函数里仅对非 CDO 实例初始化
    //   提示: HasAnyFlags(RF_ClassDefaultObject) 判断当前对象是否为 CDO
    // ══════════════════════════════════════════════════════
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExD1")
    // int32 Counter = 0;

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 声明 UFUNCTION(BlueprintCallable) void AdvanceCounter()
    //   实现在 .cpp 中, 每次调用 Counter++ 并 UE_LOG
    // ══════════════════════════════════════════════════════
    // UFUNCTION(BlueprintCallable, Category = "ExD1")
    // void AdvanceCounter();

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 声明 UFUNCTION(BlueprintCallable) void DumpProperties()
    //   用 TFieldIterator<FProperty> 遍历所有 UPROPERTY 并打印 Name + CPPType
    //   参考: Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h
    // ══════════════════════════════════════════════════════
    // UFUNCTION(BlueprintCallable, Category = "ExD1")
    // void DumpProperties();
};
