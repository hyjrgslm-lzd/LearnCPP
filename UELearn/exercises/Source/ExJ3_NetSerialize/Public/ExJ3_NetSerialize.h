// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-3
// 小节: 自定义 NetSerialize 特化 + float 压缩到 16bit
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
// 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h (USTRUCT/GENERATED_BODY)
// 参考: Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h (DOREPLIFETIME)
#include "ExJ3_NetSerialize.generated.h"

// ══════════════════════════════════════════════════════════════
// 自定义 NetSerialize struct:
// 演示把两个 float 压缩到各 16bit 整数进行传输
// 对应教材 §J-3 进阶任务: 自定义 FJ3CompressedVector
// 参考: Engine/Source/Runtime/Net/Core/Classes/Net/Serialization/FastArraySerializer.h
// ══════════════════════════════════════════════════════════════

USTRUCT(BlueprintType)
struct FExJ3CompressedData
{
    GENERATED_BODY()

    // 原始浮点值, 传输时量化压缩到 16bit 整数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "J3")
    float X;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "J3")
    float Y;

    FExJ3CompressedData() : X(0.f), Y(0.f) {}

    // ──────────────────────────────────────────────────────────
    // TODO [必做] 1: 实现 NetSerialize
    // IsSaving() == true  → 发送端: 将 float 量化为 int16 写入 FArchive
    // IsLoading() == true → 接收端: 从 FArchive 读 int16 并反量化为 float
    // 量化精度: 乘以 100.f 后取整 (误差 < 0.01)
    // 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h
    // ──────────────────────────────────────────────────────────
    bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
    {
        // TODO [必做] 1: 实现压缩传输
        // if (Ar.IsSaving())
        // {
        //     int16 QX = (int16)FMath::Clamp(FMath::RoundToInt(X * 100.f), -32768, 32767);
        //     int16 QY = (int16)FMath::Clamp(FMath::RoundToInt(Y * 100.f), -32768, 32767);
        //     Ar << QX << QY;
        // }
        // else
        // {
        //     int16 QX = 0, QY = 0;
        //     Ar << QX << QY;
        //     X = QX / 100.f;
        //     Y = QY / 100.f;
        // }
        // bOutSuccess = true;
        // return true;

        bOutSuccess = true;
        return true; // 骨架: 暂不压缩, 直接通过
    }
};

// ══════════════════════════════════════════════════════════════
// TStructOpsTypeTraits 特化: 告知引擎使用自定义 NetSerialize
// 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h
// ══════════════════════════════════════════════════════════════
template<>
struct TStructOpsTypeTraits<FExJ3CompressedData>
    : public TStructOpsTypeTraitsBase2<FExJ3CompressedData>
{
    enum { WithNetSerializer = true };
};

// ══════════════════════════════════════════════════════════════
// Actor: 持有 FExJ3CompressedData 并通过 DOREPLIFETIME 同步
// ══════════════════════════════════════════════════════════════

UCLASS(BlueprintType)
class EXJ3_NETSERIALIZE_API AExJ3NetSerializeActor : public AActor
{
    GENERATED_BODY()

public:
    AExJ3NetSerializeActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // TODO [必做] 2: 注册到 GetLifetimeReplicatedProps, 使用 DOREPLIFETIME
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "J3")
    FExJ3CompressedData CompressedPos;

    // TODO [必做] 3: 实现 GetLifetimeReplicatedProps
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // TODO [进阶] 1: 服务端每帧更新 CompressedPos, 客户端打印收到的值与误差
    // 验证量化误差是否 < 0.01

private:
    float UpdateTimer;
};
