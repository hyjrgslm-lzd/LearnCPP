// ============================================================
// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-3
// C++ 标准要求: C++20
// 本题目标: 自定义 struct NetSerialize 特化, 演示 float 压缩到 16bit 传输;
//           理解 FArchive::IsSaving() / IsLoading() 网络语义
//
// 骨架阶段预期行为: Module 注册, FExJ3CompressedData 结构体可编译
// 完成后预期行为 (PIE 双人模式):
//   服务端: [J3] Server: sending CompressedPos X=12.34 Y=56.78
//   客户端: [J3] Client: received CompressedPos X=12.34 Y=56.78 (误差 < 0.01)
// ============================================================

#include "ExJ3_NetSerialize.h"
#include "Modules/ModuleManager.h"
// DOREPLIFETIME: Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h
#include "Net/UnrealNetwork.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ExJ3_NetSerialize);

DEFINE_LOG_CATEGORY_STATIC(LogExJ3, Log, All);

// ------------------------------------------------------------
// 构造函数
// ------------------------------------------------------------
AExJ3NetSerializeActor::AExJ3NetSerializeActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetReplicates(true);

    UpdateTimer = 0.f;
}

// ------------------------------------------------------------
// GetLifetimeReplicatedProps: 注册 CompressedPos
// 引擎检测到 WithNetSerializer=true 后会调用 FExJ3CompressedData::NetSerialize
// 而非默认的逐字段比较, 带宽更低
// ------------------------------------------------------------
void AExJ3NetSerializeActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 注册 CompressedPos
    // ══════════════════════════════════════════════════════
    DOREPLIFETIME(AExJ3NetSerializeActor, CompressedPos);
}

// ------------------------------------------------------------
// BeginPlay
// ------------------------------------------------------------
void AExJ3NetSerializeActor::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogExJ3, Log, TEXT("[J3] Actor=%s Role=%d HasAuth=%d"),
        *GetName(),
        (int32)GetLocalRole(),
        (int32)HasAuthority());
}

// ------------------------------------------------------------
// Tick: 服务端每 1 秒更新 CompressedPos, 客户端打印收到的值
// ------------------------------------------------------------
void AExJ3NetSerializeActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (HasAuthority())
    {
        // ══════════════════════════════════════════════════
        // TODO [必做] 2: 服务端每 1 秒更新 CompressedPos
        // 用不同 X/Y 值验证 NetSerialize 压缩精度
        // ══════════════════════════════════════════════════
        UpdateTimer += DeltaTime;
        if (UpdateTimer >= 1.f)
        {
            UpdateTimer = 0.f;

            // 生成测试值 (范围 [-327.67, 327.67], 对应 int16 * 0.01)
            CompressedPos.X = FMath::Sin(GetWorld()->GetTimeSeconds()) * 100.f;
            CompressedPos.Y = FMath::Cos(GetWorld()->GetTimeSeconds()) * 100.f;

            UE_LOG(LogExJ3, Log, TEXT("[J3] Server: sending CompressedPos X=%.4f Y=%.4f"),
                CompressedPos.X, CompressedPos.Y);
        }
    }
    else
    {
        // ══════════════════════════════════════════════════
        // TODO [必做] 3: 客户端每帧打印收到的值
        // 验证量化误差: |接收值 - 发送值| < 0.01
        // ══════════════════════════════════════════════════

        // 骨架: 仅每 60 帧打印一次避免刷屏
        static int32 FrameCount = 0;
        if (++FrameCount % 60 == 0)
        {
            UE_LOG(LogExJ3, Log, TEXT("[J3] Client: received CompressedPos X=%.4f Y=%.4f"),
                CompressedPos.X, CompressedPos.Y);
        }
    }
}

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 在 ExJ3_NetSerialize.h 里完整实现 NetSerialize
//   发送端: QX = (int16)clamp(round(X * 100), -32768, 32767); Ar << QX;
//   接收端: Ar << QX; X = QX / 100.f;
// 验证: 服务端 X=12.345 → 传输 QX=1235 → 接收端 X=12.35 (误差 0.005)
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 2: 对照 E 模块的 FArchive 双向语义
// IsSaving() == true  → 网络发送端打包 (等价 E 模块"写入方向")
// IsLoading() == true → 网络接收端解包 (等价 E 模块"读取方向")
// 同一份代码跑两个方向, 这是 FArchive 设计的核心价值
// ══════════════════════════════════════════════════════

// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// float Sent = 12.345f;
// float Received = /* 接收端解量化后的值 */;
// ensureMsgf(FMath::Abs(Sent - Received) < 0.02f,
//     TEXT("量化误差超限: 期望 < 0.02, 实际 = %.4f"), FMath::Abs(Sent - Received));
