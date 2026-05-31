// ============================================================
// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-2
// C++ 标准要求: C++20
// 本题目标: 手写完整 replicated property (含 DOREPLIFETIME),
//           实现 RepNotify 回调, 各写一个 Server/Client/NetMulticast RPC
//
// 骨架阶段预期行为: Module 注册, Actor 可放入关卡, 三种 RPC 骨架编译通过
// 完成后预期行为 (PIE 双人模式, Number of Players=2):
//   服务端: [J2] Server: Health=90.0 after damage
//   客户端: [J2] Client OnRep_Health called, new Health=90.0
//   客户端: [J2] Client received: you are dead  (Health 降至 0 时)
//   两端:   [J2] Multicast on Role=3 (服务端) / Role=2 (客户端)
// ============================================================

#include "ExJ2_Replication.h"
#include "Modules/ModuleManager.h"
// DOREPLIFETIME 宏: Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h (259行)
#include "Net/UnrealNetwork.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ExJ2_Replication);

DEFINE_LOG_CATEGORY_STATIC(LogExJ2, Log, All);

// ------------------------------------------------------------
// 构造函数
// SetReplicates(true) 在构造阶段等价于 bReplicates = true
// 参考: Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h (553行)
// ------------------------------------------------------------
AExJ2ReplicatedActor::AExJ2ReplicatedActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetReplicates(true);

    Health = 100.f;
    DamageTimer = 0.f;
}

// ------------------------------------------------------------
// GetLifetimeReplicatedProps: 向 FLifetimeProperty 数组注册需要同步的字段
// 必须调用 Super, 否则基类的 ReplicatedMovement 等字段不会注册
// 参考: Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h (259行)
// ------------------------------------------------------------
void AExJ2ReplicatedActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 注册 Health 属性
    // ══════════════════════════════════════════════════════
    DOREPLIFETIME(AExJ2ReplicatedActor, Health);

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 改用条件同步 DOREPLIFETIME_CONDITION
    // 只同步给 Owner 客户端:
    // DOREPLIFETIME_CONDITION(AExJ2ReplicatedActor, Health, COND_OwnerOnly);
    // 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/CoreNetTypes.h (21-38行)
    // ══════════════════════════════════════════════════════
}

// ------------------------------------------------------------
// BeginPlay: 打印当前 Role 状态, 验证服务端/客户端 Role 值
// 服务端: Role=3 (ROLE_Authority), 客户端: Role=2 (ROLE_SimulatedProxy)
// ------------------------------------------------------------
void AExJ2ReplicatedActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogExJ2, Log, TEXT("[J2] Actor=%s Role=%d RemoteRole=%d HasAuth=%d"),
        *GetName(),
        (int32)GetLocalRole(),
        (int32)GetRemoteRole(),
        (int32)HasAuthority());
}

// ------------------------------------------------------------
// Tick: 服务端每 2 秒发起一次 Multicast, 演示广播 RPC
// ------------------------------------------------------------
void AExJ2ReplicatedActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HasAuthority())
    {
        return; // 客户端只接收同步, 不驱动逻辑
    }

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 服务端每 2 秒调用 ServerRequestDamage 减少 Health
    // 演示 RepNotify 在客户端被调用
    // ══════════════════════════════════════════════════════
    DamageTimer += DeltaTime;
    if (DamageTimer >= 2.f)
    {
        DamageTimer = 0.f;

        // 服务端直接修改 Health (无需走 RPC, 已有 Authority)
        Health = FMath::Max(0.f, Health - 10.f);
        UE_LOG(LogExJ2, Log, TEXT("[J2] Server: Health reduced to %.1f"), Health);

        if (Health <= 0.f)
        {
            // 通知拥有该 Actor 的客户端已死亡
            ClientNotifyDead();
            // 广播爆炸特效
            MulticastExplode();
        }
    }
}

// ------------------------------------------------------------
// OnRep_Health: 客户端收到新 Health 值后由引擎自动调用
// 注意: 服务端不会调用此函数
// ------------------------------------------------------------
void AExJ2ReplicatedActor::OnRep_Health()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 在此处理客户端收到新 Health 后的 UI/效果更新
    // ══════════════════════════════════════════════════════
    UE_LOG(LogExJ2, Log, TEXT("[J2] Client OnRep_Health called, new Health=%.1f"), Health);

    // ---- 验证区 ----
    // ensureMsgf(!HasAuthority(), TEXT("OnRep_Health 不应在服务端被调用"));
}

// ------------------------------------------------------------
// ServerRequestDamage_Implementation: 在服务端 GameThread 执行
// 触发条件: 客户端 (ROLE_AutonomousProxy) 调用
// _Implementation 后缀由 UHT 生成, 业务逻辑必须写在此函数里
// ------------------------------------------------------------
void AExJ2ReplicatedActor::ServerRequestDamage_Implementation(float Amount)
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 完整实现扣血逻辑
    // ══════════════════════════════════════════════════════
    check(HasAuthority()); // Server RPC 执行时必须有 Authority

    Health = FMath::Max(0.f, Health - Amount);
    UE_LOG(LogExJ2, Log, TEXT("[J2] Server ServerRequestDamage_Implementation: Amount=%.1f Health=%.1f"),
        Amount, Health);

    // ---- 验证区 ----
    // ensureMsgf(Health >= 0.f, TEXT("Health 不能为负值"));
}

// ------------------------------------------------------------
// ClientNotifyDead_Implementation: 在拥有该 Actor 的客户端 GameThread 执行
// 注意: Client RPC 不广播, 只发往 Owner 对应的客户端
// ------------------------------------------------------------
void AExJ2ReplicatedActor::ClientNotifyDead_Implementation()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 5: 在此处理客户端死亡 UI 逻辑
    // ══════════════════════════════════════════════════════
    UE_LOG(LogExJ2, Warning, TEXT("[J2] Client received: you are dead"));
}

// ------------------------------------------------------------
// MulticastExplode_Implementation: 在服务端和所有客户端均执行
// 打印 GetLocalRole() 以验证两端均收到
// ------------------------------------------------------------
void AExJ2ReplicatedActor::MulticastExplode_Implementation()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 6: 在此播放爆炸特效 (骨架: 仅打印日志)
    // ══════════════════════════════════════════════════════
    UE_LOG(LogExJ2, Log, TEXT("[J2] MulticastExplode on Role=%d"),
        (int32)GetLocalRole());
}

// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// ensureMsgf(GetLocalRole() == ROLE_Authority || GetLocalRole() == ROLE_SimulatedProxy,
//     TEXT("Actor 应处于 Authority 或 SimulatedProxy 状态"));
