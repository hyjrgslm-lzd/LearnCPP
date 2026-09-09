// ============================================================
// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-4
// C++ 标准要求: C++20
// 本题目标: 展示客户端预测+服务端校正骨架; 三类 RPC 完整编译验证
//           不实现真实校正逻辑, 只展示 ServerMove/ClientCorrect/MulticastExplode
//           与对应 _Implementation 的编译关系
//
// 骨架阶段预期行为: Module 注册, 三种 RPC 骨架编译通过
// 完成后预期行为 (PIE 双人模式):
//   客户端(每 0.5s): [J4] Client sending ServerMove Loc=(X Y Z)
//   服务端:          [J4] Server ServerMove_Impl received Loc=(X Y Z)
//   客户端:          [J4] Client ClientCorrect_Impl received Correct=(X Y Z)
//   两端:            [J4] MulticastExplode on Role=3/2
// ============================================================

#include "ExJ4_NetPrediction.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ExJ4_NetPrediction);

DEFINE_LOG_CATEGORY_STATIC(LogExJ4, Log, All);

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
AExJ4PredictionActor::AExJ4PredictionActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetReplicates(true);

    PredictedLocation = FVector::ZeroVector;
    MoveTimer = 0.f;
}

// ------------------------------------------------------------
// BeginPlay: 打印 Role 状态
// 服务端 Role=3 (ROLE_Authority)
// 客户端 Role=2 (ROLE_SimulatedProxy) 或 Role=1 (ROLE_AutonomousProxy, 若由本地玩家控制)
// ------------------------------------------------------------
void AExJ4PredictionActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogExJ4, Log, TEXT("[J4] Actor=%s Role=%d RemoteRole=%d HasAuth=%d"),
        *GetName(),
        (int32)GetLocalRole(),
        (int32)GetRemoteRole(),
        (int32)HasAuthority());
}

// ------------------------------------------------------------
// Tick: 客户端每 0.5 秒发送一次 ServerMove (模拟输入上报)
//       服务端每 2 秒触发一次 MulticastExplode
// ------------------------------------------------------------
void AExJ4PredictionActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    MoveTimer += DeltaTime;

    if (!HasAuthority())
    {
        // ══════════════════════════════════════════════════
        // TODO [必做] 1: 客户端每 0.5 秒调用 ServerMove
        // 模拟输入上报: 预测位置 = 当前位置 + 微小偏移
        // 参考: CharacterMovementComponent.h CallServerMovePacked (2379行)
        // ══════════════════════════════════════════════════
        if (MoveTimer >= 0.5f)
        {
            MoveTimer = 0.f;

            // 骨架: 用简单累加模拟移动输入
            PredictedLocation += FVector(10.f, 0.f, 0.f);

            UE_LOG(LogExJ4, Log, TEXT("[J4] Client sending ServerMove Loc=%s"),
                *PredictedLocation.ToString());

            ServerMove(FVector_NetQuantize(PredictedLocation));
        }
    }
    else
    {
        // ══════════════════════════════════════════════════
        // TODO [必做] 2: 服务端每 2 秒触发 MulticastExplode
        // ══════════════════════════════════════════════════
        if (MoveTimer >= 2.f)
        {
            MoveTimer = 0.f;
            MulticastExplode();
        }
    }
}

// ------------------------------------------------------------
// ServerMove_Implementation: 在服务端 GameThread 执行
// 触发条件: 客户端(ROLE_AutonomousProxy) 调用 ServerMove()
// 服务端收到客户端上报的位置后, 比较与服务端模拟位置的偏差
// 若偏差超阈值则调用 ClientCorrect 下发校正
// 参考: CharacterMovementComponent.h ServerMoveHandleClientError (2387行)
// ------------------------------------------------------------
void AExJ4PredictionActor::ServerMove_Implementation(FVector_NetQuantize Loc)
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 实现服务端接收位置 + 简单偏差检测
    // 骨架: 直接信任客户端位置, 设置服务端 Actor 位置
    // ══════════════════════════════════════════════════════
    check(HasAuthority());

    UE_LOG(LogExJ4, Log, TEXT("[J4] Server ServerMove_Impl received Loc=%s"),
        *FVector(Loc).ToString());

    // 骨架: 服务端接受客户端位置 (真实预测应做服务端独立模拟后比较)
    SetActorLocation(FVector(Loc));

    // 发回校正 (骨架: 直接用接收到的位置作为"服务端权威位置")
    ClientCorrect(FVector(Loc));

    // ---- 验证区 ----
    // ensureMsgf(HasAuthority(), TEXT("ServerMove_Implementation 必须在服务端执行"));
}

// ------------------------------------------------------------
// ClientCorrect_Implementation: 在目标客户端 GameThread 执行
// 触发条件: 服务端调用 ClientCorrect()
// 客户端收到校正后应: 1) 重置位置到服务端权威位置 2) 重播 SavedMoves 队列
// 参考: CharacterMovementReplication.h ClientAdjustPosition 注释 (252行)
// ------------------------------------------------------------
void AExJ4PredictionActor::ClientCorrect_Implementation(FVector Correct)
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 实现客户端校正
    // 真实实现: 重置到 Correct 位置 + 重播历史输入 (SavedMoves)
    // 骨架: 仅打印校正位置
    // ══════════════════════════════════════════════════════
    UE_LOG(LogExJ4, Log, TEXT("[J4] Client ClientCorrect_Impl received Correct=%s"),
        *Correct.ToString());

    // 骨架: 直接应用校正位置
    PredictedLocation = Correct;
}

// ------------------------------------------------------------
// MulticastExplode_Implementation: 在服务端和所有客户端均执行
// Unreliable: 允许丢包, 视觉特效可以接受偶发丢失
// ------------------------------------------------------------
void AExJ4PredictionActor::MulticastExplode_Implementation()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 5: 在此播放爆炸特效 (骨架: 仅打印 Role)
    // ══════════════════════════════════════════════════════
    UE_LOG(LogExJ4, Log, TEXT("[J4] MulticastExplode on Role=%d"),
        (int32)GetLocalRole());
}

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 阅读 CharacterMovementComponent.h (2350-2371行)
//   理解 FSavedMovePtr = TSharedPtr<FSavedMove_Character> 为何用 TSharedPtr:
//   - FSavedMove_Character 不是 UObject, 不受 GC 管理
//   - 每帧 new/delete, 引用计数自动管理生命周期 (模块 C 的 use 阶梯)
//   - Server ack 到达后从队列删除, 引用计数归零自动析构
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 2: 在 PIE Console 输入 p.NetShowCorrections 1
//   观察客户端被服务端校正时的红色箭头 (橡皮筋效果)
// ══════════════════════════════════════════════════════

// ---- 验证区 ----
// ensureMsgf(GetLocalRole() != ROLE_None, TEXT("Actor Role 不应为 ROLE_None"));
