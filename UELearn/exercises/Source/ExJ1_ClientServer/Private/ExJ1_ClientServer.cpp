// ============================================================
// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-1
// C++ 标准要求: C++20
// 本题目标: PIE 多人模式下观察 HasAuthority() / Role 区别
//
// 骨架阶段预期行为 (PIE 2-player Dedicated Server 模式启动后 Output Log):
//   [Server] LogExJ1: [J1] Actor=ExJ1ServerSpawned_0 Role=3 RemoteRole=1 HasAuth=1 → Server
//   [Client] LogExJ1: [J1] Actor=ExJ1ServerSpawned_0 Role=1 RemoteRole=3 HasAuth=0 → Client
//
// Role 值对照:
//   ROLE_Authority       = 3  (ENetRole::ROLE_Authority)
//   ROLE_AutonomousProxy = 2  (ENetRole::ROLE_AutonomousProxy)
//   ROLE_SimulatedProxy  = 1  (ENetRole::ROLE_SimulatedProxy)
//   ROLE_None            = 0
//
// PIE 配置步骤 (必读，不得跳过):
//   1. 点击 Play 按钮旁的下拉箭头 → Advanced Settings
//   2. Number of Players = 2
//   3. Net Mode = Play As Dedicated Server
//   4. 点击 Play
//   5. Output Log 面板下拉菜单切换 "Server" / "Client 1" 分别查看日志
// ============================================================

#include "ExJ1_ClientServer.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ExJ1_ClientServer);

DEFINE_LOG_CATEGORY_STATIC(LogExJ1, Log, All);

// ============================================================
// AExJ1ServerSpawned 实现
// ============================================================

AExJ1ServerSpawned::AExJ1ServerSpawned()
{
    // bReplicates = true: 构造阶段标记，将此 Actor 加入 NetDriver 的复制列表
    // 等价于在 BeginPlay 后调用 SetReplicates(true)
    // 区别: 构造阶段设置是推荐做法；SetReplicates() 应在 World 完全初始化后调用
    // 参考: Actor.h 第 722 行注释
    bReplicates = true;

    // 开启 Tick，供 TODO [必做] 1 使用 (服务端移动演示)
    PrimaryActorTick.bCanEverTick = true;

    UE_LOG(LogExJ1, Log, TEXT("AExJ1ServerSpawned 构造函数, bReplicates=true"));
}

void AExJ1ServerSpawned::BeginPlay()
{
    Super::BeginPlay();

    // ── 核心教学内容 ─────────────────────────────────────
    //
    // HasAuthority() == true  → 当前端是服务端 (此 Actor 的权威副本在此)
    // HasAuthority() == false → 当前端是客户端 (此 Actor 是 SimulatedProxy)
    //
    // GetLocalRole()  → 本端看到此 Actor 的 Role
    //   服务端: ROLE_Authority (3)
    //   客户端: ROLE_SimulatedProxy (1)
    //
    // GetRemoteRole() → 对端看到此 Actor 的 Role
    //   服务端: ROLE_SimulatedProxy (1) — 服务端认为客户端持有 SimulatedProxy
    //   客户端: ROLE_Authority (3)      — 客户端知道服务端持有 Authority

    if (HasAuthority())
    {
        UE_LOG(LogExJ1, Warning,
            TEXT("[J1] Actor=%s Role=%d RemoteRole=%d HasAuth=%d → Server"),
            *GetName(),
            (int32)GetLocalRole(),
            (int32)GetRemoteRole(),
            (int32)HasAuthority());
    }
    else
    {
        UE_LOG(LogExJ1, Warning,
            TEXT("[J1] Actor=%s Role=%d RemoteRole=%d HasAuth=%d → Client"),
            *GetName(),
            (int32)GetLocalRole(),
            (int32)GetRemoteRole(),
            (int32)HasAuthority());
    }

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 在 GameMode::BeginPlay 或 ConsoleCommand 里:
    //   if (HasAuthority())  // GameMode 只在服务端存在
    //   {
    //       GetWorld()->SpawnActor<AExJ1ServerSpawned>(
    //           AExJ1ServerSpawned::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    //   }
    //   观察: 服务端 SpawnActor 后，客户端日志自动出现 → Actor Channel 自动创建
    // ══════════════════════════════════════════════════════
}

void AExJ1ServerSpawned::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 仅在服务端移动 Actor (HasAuthority() 守卫)
    //
    // if (HasAuthority())
    // {
    //     // 服务端每帧沿 X 轴移动 100 cm/s
    //     FVector NewLoc = GetActorLocation() + FVector(100.f * DeltaTime, 0.f, 0.f);
    //     SetActorLocation(NewLoc);
    // }
    // else
    // {
    //     // 客户端仅观察位置 (由 ReplicatedMovement 自动同步)
    //     // AActor::ReplicatedMovement 是 UPROPERTY(ReplicatedUsing=OnRep_ReplicatedMovement)
    //     // Actor.cpp 第 2014 行: DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(AActor, ReplicatedMovement, ...)
    //     // 无需手写 DOREPLIFETIME — 基类已注册
    //     UE_LOG(LogExJ1, Verbose,
    //         TEXT("[J1 Client] Actor=%s X=%.1f"), *GetName(), GetActorLocation().X);
    // }
    //
    // 常见坑: 客户端对 ROLE_SimulatedProxy 的 Actor 调用 SetActorLocation
    //         → 下次 replication tick 被服务端值覆盖 (客户端无 Authority)
    // ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 降低 NetUpdateFrequency 观察客户端位置更新卡顿
//   在构造函数里: NetUpdateFrequency = 2.0f;  // 默认约 100.0f
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 2: 切换 Net Mode = Play As Listen Server
//   观察: Listen Server 的本地玩家 Pawn
//     服务端侧: Role=ROLE_Authority
//     控制此 Pawn 的客户端侧: Role=ROLE_AutonomousProxy
//     其他客户端侧: Role=ROLE_SimulatedProxy
// ══════════════════════════════════════════════════════

// ---- 验证区 (完成 TODO 后把下列 ensure 移入对应代码路径) ----
// ensureMsgf(bReplicates, TEXT("bReplicates 应在构造时设置为 true"));
// ensureMsgf(HasAuthority() || GetLocalRole() == ROLE_SimulatedProxy,
//     TEXT("非 Authority 端 Role 应为 SimulatedProxy"));
