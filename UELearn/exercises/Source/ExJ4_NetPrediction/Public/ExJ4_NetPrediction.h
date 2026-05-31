// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-4
// 小节: 网络预测骨架 — 三类 RPC 展示 (Server / Client / NetMulticast)
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
// 参考: Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h (Server, Reliable 宏)
// 参考: Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h (HasAuthority)
#include "ExJ4_NetPrediction.generated.h"

// ══════════════════════════════════════════════════════════════
// J4 重点: 展示三类 RPC 的声明与 _Implementation 后缀约定
// 本骨架不实现真实的预测/校正逻辑, 只验证三向 RPC 的编译正确性
// 真实预测见: Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h
//   prediction 注释 (2350-2371行), CallServerMovePacked (2379行)
// ══════════════════════════════════════════════════════════════

UCLASS(BlueprintType)
class EXJ4_NETPREDICTION_API AExJ4PredictionActor : public AActor
{
    GENERATED_BODY()

public:
    AExJ4PredictionActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // ──────────────────────────────────────────────────────────
    // 必做任务 1: Server RPC
    // 触发条件: 客户端(ROLE_AutonomousProxy)调用 → 在服务端执行
    // FVector_NetQuantize: UE 专用压缩位置类型, 减少带宽
    // 参考: Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h
    // ──────────────────────────────────────────────────────────
    UFUNCTION(Server, Reliable)
    void ServerMove(FVector_NetQuantize Loc);

    // TODO [必做] 1: 实现 ServerMove_Implementation
    // 该函数在服务端 GameThread 执行, check(HasAuthority()) 应在函数体内验证
    // 骨架实现: 打印收到的位置 + 调用 ClientCorrect 发回校正

    // ──────────────────────────────────────────────────────────
    // 必做任务 2: Client RPC
    // 触发条件: 服务端调用 → 在拥有该 Actor 的客户端执行
    // 对应 CharacterMovementComponent 的 ClientAdjustPosition
    // ──────────────────────────────────────────────────────────
    UFUNCTION(Client, Reliable)
    void ClientCorrect(FVector Correct);

    // TODO [必做] 2: 实现 ClientCorrect_Implementation
    // 该函数在客户端 GameThread 执行, 骨架实现: 打印校正位置

    // ──────────────────────────────────────────────────────────
    // 必做任务 3: NetMulticast RPC
    // 触发条件: 服务端调用 → 在服务端和所有客户端均执行
    // Unreliable: 允许丢包, 适合视觉特效等非关键信息
    // ──────────────────────────────────────────────────────────
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastExplode();

    // TODO [必做] 3: 实现 MulticastExplode_Implementation
    // 打印 GetLocalRole() 以验证在服务端和客户端均执行

    // ──────────────────────────────────────────────────────────
    // TODO [进阶] 1: 阅读 CharacterMovementComponent.h 预测注释 (2350-2371行)
    // 画出 client input tick → ServerMove → ClientAdjustPosition 时序图
    // 理解 FSavedMovePtr(TSharedPtr<FSavedMove_Character>) 为何用 TSharedPtr 而非 UObject
    // ──────────────────────────────────────────────────────────

    // ──────────────────────────────────────────────────────────
    // TODO [进阶] 2: 用 p.NetShowCorrections 1 在 PIE 中观察橡皮筋校正效果
    // ──────────────────────────────────────────────────────────

private:
    // 客户端本地预测位置 (骨架: 仅演示数据存储)
    FVector PredictedLocation;

    // 计时器: 客户端每隔 0.5 秒发送一次 ServerMove
    float MoveTimer;
};
