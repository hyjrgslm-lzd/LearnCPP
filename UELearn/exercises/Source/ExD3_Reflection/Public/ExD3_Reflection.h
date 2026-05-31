// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-3
// 小节: TFieldIterator<FProperty> 遍历 PropertyLink / CastField / 反射写值
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
// TFieldIterator 定义在 FieldIterator.h
#include "UObject/FieldIterator.h"
// FProperty 子类型 (FBoolProperty/FIntProperty/...) 定义在 UnrealType.h
#include "UObject/UnrealType.h"
#include "ExD3_Reflection.generated.h"

// ══════════════════════════════════════════════════════
// 包含多种字段类型的 UCLASS, 用于反射遍历演示
// ══════════════════════════════════════════════════════
UCLASS()
class EXD3_REFLECTION_API UExD3ReflectTarget : public UObject
{
    GENERATED_BODY()

public:
    // TODO [必做] 1: 取消注释以下所有 UPROPERTY, 使 UHT 为其生成 FProperty 描述符
    // UPROPERTY(EditAnywhere, Category = "ExD3") bool   bActive  = true;
    // UPROPERTY(EditAnywhere, Category = "ExD3") int32  Count    = 0;
    // UPROPERTY(EditAnywhere, Category = "ExD3") float  Weight   = 1.f;
    // UPROPERTY(EditAnywhere, Category = "ExD3") FString Label   = TEXT("hello");
    // UPROPERTY(EditAnywhere, Category = "ExD3") TObjectPtr<UObject> ChildRef;
    // UPROPERTY(EditAnywhere, Category = "ExD3") TArray<int32> Numbers;

    // 故意不加 UPROPERTY 的字段 —— GC 不可见, PropertyLink 中不出现
    int32 HiddenField = 42;

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 取消注释并实现 OnPropertyChanged
    // UFUNCTION(Category = "ExD3")
    // void OnPropertyChanged();
    // ══════════════════════════════════════════════════════
};

// ══════════════════════════════════════════════════════
// 工具函数: 遍历并打印一个 UClass 的所有 UPROPERTY
// 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:211
//       FProperty::PropertyLinkNext (链表 next 指针)
// ══════════════════════════════════════════════════════

// 方式 1: 手动遍历 PropertyLink 链
void ExD3_DumpClassPropertiesManual(UClass* Class);

// 方式 2: 使用 TFieldIterator (UE 推荐方式, 自动处理继承链)
void ExD3_DumpClassPropertiesIterator(UClass* Class);

// 通过反射写入 ChildRef 字段并验证读取
void ExD3_WriteAndReadViaReflection();
