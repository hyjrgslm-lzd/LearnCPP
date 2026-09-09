// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-2
// 小节: Replicated UPROPERTY + RepNotify + RPC 三向
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExJ2_Replication.generated.h"

UCLASS(BlueprintType)
class EXJ2_REPLICATION_API AExJ2ReplicatedActor : public AActor
{
    GENERATED_BODY()

public:
    AExJ2ReplicatedActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // ──────────────────────────────────────────────────────────
    // 必做任务 1: Replicated property + RepNotify
    // 教材参考: Actor.h §GetLifetimeReplicatedProps, UnrealNetwork.h §DOREPLIFETIME
    // ──────────────────────────────────────────────────────────

    // Health 属性: 声明 ReplicatedUsing, 客户端收到新值后引擎自动调用 OnRep_Health
    // 参考: Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h (273行)
    UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "J2|Replication")
    float Health;

    // RepNotify 回调: 注意它只在接收端(客户端)被调用, 服务端不会调用
    UFUNCTION()
    void OnRep_Health();

    // TODO [必做] 1: 在 GetLifetimeReplicatedProps 里用 DOREPLIFETIME 注册 Health
    // 参考: Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h (259行)
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ──────────────────────────────────────────────────────────
    // 必做任务 2: Server RPC
    // 只能从拥有权限的客户端(ROLE_AutonomousProxy)调用, 在服务端执行
    // _Implementation 后缀由 UHT 生成, 业务逻辑写在此函数里
    // ──────────────────────────────────────────────────────────
    UFUNCTION(Server, Reliable)
    void ServerRequestDamage(float Amount);

    // TODO [必做] 2: 在 .cpp 里实现 ServerRequestDamage_Implementation
    // 注意 check(HasAuthority()) 应在函数体内验证

    // ──────────────────────────────────────────────────────────
    // 必做任务 3: Client RPC
    // 只能从服务端调用, 在拥有该 Actor 的目标客户端执行
    // ──────────────────────────────────────────────────────────
    UFUNCTION(Client, Reliable)
    void ClientNotifyDead();

    // TODO [必做] 3: 在 .cpp 里实现 ClientNotifyDead_Implementation
    // 注意: Client RPC 不是广播, 只发往 Owner 客户端

    // ──────────────────────────────────────────────────────────
    // 必做任务 4: NetMulticast RPC
    // 从服务端调用, 在服务端和所有连接的客户端均执行
    // ──────────────────────────────────────────────────────────
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastExplode();

    // TODO [必做] 4: 在 .cpp 里实现 MulticastExplode_Implementation
    // 对比 Reliable vs Unreliable: 前者保证送达, 后者允许丢包

private:
    // 服务端每帧累积计时器, 用于定时扣血演示
    float DamageTimer;
};
