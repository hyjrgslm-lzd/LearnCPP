// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-2
// 小节: CreateDefaultSubobject 只能在构造函数内 / CDO 观察 / TObjectPtr access barrier
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ExD2_CDOSubobject.generated.h"

// ══════════════════════════════════════════════════════
// 子对象类: 演示 CreateDefaultSubobject 创建的子对象
// ══════════════════════════════════════════════════════
UCLASS()
class EXD2_CDOSUBOBJECT_API UExD2EngineSpec : public UObject
{
    GENERATED_BODY()

public:
    // TODO [必做] 1: 声明 UPROPERTY(EditAnywhere) float Horsepower = 400.f
    // UPROPERTY(EditAnywhere, Category = "ExD2")
    // float Horsepower = 400.f;
};

// ══════════════════════════════════════════════════════
// 宿主类: 在构造函数里 CreateDefaultSubobject<UExD2EngineSpec>
// ══════════════════════════════════════════════════════
UCLASS()
class EXD2_CDOSUBOBJECT_API UExD2VehicleConfig : public UObject
{
    GENERATED_BODY()

public:
    // FObjectInitializer 构造函数 (DoNotCreateDefaultSubobject 需要此形式)
    // 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1363
    UExD2VehicleConfig(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 声明 TObjectPtr<UExD2EngineSpec> EngineSpec (UPROPERTY)
    //   在构造函数里使用 CreateDefaultSubobject<UExD2EngineSpec>(TEXT("EngineSpec"))
    //   注意: CreateDefaultSubobject 只能在构造函数内调用!
    //         在构造函数外调用会触发 ensure(CurrentInitializer) 失败
    // ══════════════════════════════════════════════════════
    // UPROPERTY(EditAnywhere, Category = "ExD2")
    // TObjectPtr<UExD2EngineSpec> EngineSpec;

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 声明 UPROPERTY(EditAnywhere) int32 WheelCount = 4
    // ══════════════════════════════════════════════════════
    // UPROPERTY(EditAnywhere, Category = "ExD2")
    // int32 WheelCount = 4;
};

// ══════════════════════════════════════════════════════
// 进阶: 派生类演示 DoNotCreateDefaultSubobject
// TODO [进阶] 1: 声明 UExD2LightVehicle : public UExD2VehicleConfig
//   在其构造函数里调用 ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("EngineSpec"))
//   观察 CDO 的 EngineSpec 是否为 null
// ══════════════════════════════════════════════════════
// UCLASS()
// class EXD2_CDOSUBOBJECT_API UExD2LightVehicle : public UExD2VehicleConfig
// {
//     GENERATED_BODY()
// public:
//     UExD2LightVehicle(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
// };
