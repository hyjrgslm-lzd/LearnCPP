// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-1
// 小节: Dedicated Server + Client 最小工程 + Actor Replication
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ExJ1_ClientServer.generated.h"

// ============================================================
// ExJ1_ClientServer 骨架
//
// 教学重点:
//   1. bReplicates = true   — 构造阶段标记，使 Actor 加入 NetDriver 复制列表
//   2. HasAuthority()       — 区分 Server (true) 与 Client (false) 的标准方式
//   3. 三种 Role 值:
//        ROLE_Authority       (3) — 服务端拥有该 Actor 的权威副本
//        ROLE_AutonomousProxy (2) — 本地玩家控制的 Pawn 在客户端上的 Role
//        ROLE_SimulatedProxy  (1) — 其他玩家/服务端生成的 Actor 在客户端上的 Role
//   4. PIE 多人模式 (README 有详细步骤):
//        Play → 下拉箭头 → Advanced Settings → Number of Players = 2
//        → Net Mode = Play As Dedicated Server
//      观察 Output Log: 切换 "Server" / "Client" 查看两侧各自的日志
//
// 反射可见性图:
//   AExJ1ServerSpawned
//     ├── bReplicates            (非 UPROPERTY，C++ 位字段，构造时设 true)
//     ├── GetLocalRole()         → 服务端=ROLE_Authority, 客户端=ROLE_SimulatedProxy
//     ├── GetRemoteRole()        → 服务端=ROLE_SimulatedProxy, 客户端=ROLE_Authority
//     └── ReplicatedMovement     ← AActor 基类已注册，无需手写 DOREPLIFETIME
//
// 源码校对路径:
//   Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h
//     HasAuthority()   第 1941 / 4964 行
//     bReplicates      第 553 / 556 行
//     SetReplicates    第 722 行
//     GetLocalRole()   / GetRemoteRole()
// ============================================================

/**
 * AExJ1ServerSpawned
 *
 * 最小 replicated Actor，演示:
 *   - 构造函数: bReplicates = true
 *   - BeginPlay: 打印 HasAuthority() / GetLocalRole() / GetRemoteRole()
 *     → 服务端日志 "Server"，客户端日志 "Client"
 *
 * 用法:
 *   在 GameMode::BeginPlay 或 CheatManager 里仅在服务端 SpawnActor<AExJ1ServerSpawned>()。
 *   客户端无需任何代码，Actor 会自动通过 UActorChannel 复制出现。
 */
UCLASS()
class EXJ1_CLIENTSERVER_API AExJ1ServerSpawned : public AActor
{
    GENERATED_BODY()

public:
    AExJ1ServerSpawned();

protected:
    // ── AActor 生命周期钩子 ───────────────────────────────

    /**
     * BeginPlay
     *
     * 在服务端: HasAuthority() == true → 打印 "Server"
     * 在客户端: HasAuthority() == false → 打印 "Client"
     *
     * 同时打印 GetLocalRole() 和 GetRemoteRole() 供对照观察。
     *
     * 关键: 客户端的 BeginPlay 在 Actor 通过 UActorChannel 复制到达客户端时触发，
     *       不是本地 SpawnActor 触发的 (客户端没有权限 SpawnActor)。
     */
    virtual void BeginPlay() override;

    /**
     * Tick
     *
     * TODO [必做] 1: 用 if (HasAuthority()) 守卫，仅在服务端调用 SetActorLocation
     *               沿 X 轴每帧移动 1 cm，观察客户端位置随之更新
     *               (依赖 AActor::ReplicatedMovement 默认已注册 replication)
     *
     * TODO [必做] 2: 在客户端 Tick 里打印 GetActorLocation().X
     *               观察位置值是否跟随服务端变化 (约 1/NetUpdateFrequency 秒延迟)
     *
     * 常见坑: 客户端对 ROLE_SimulatedProxy Actor 调用 SetActorLocation
     *         → 下次 replication tick 被服务端值覆盖 (客户端无 Authority)
     */
    virtual void Tick(float DeltaTime) override;

    // TODO [进阶] 1: 将 NetUpdateFrequency 调低为 2.0f，观察客户端位置更新卡顿
    //               理解 NetDriver 的更新率控制
    // TODO [进阶] 2: 切换 Net Mode = Play As Listen Server，再观察 Role 打印值
    //               Listen Server 的本地玩家 Pawn 在服务端侧是 ROLE_Authority
};
