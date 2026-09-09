// ============================================================
// 对应章节: ../../../07-模块E-资产与加载.md §练习 E-1
// C++ 标准要求: C++20
// 本题目标: 用 FMemoryWriter/FMemoryReader 对 UObject 做往返序列化，
//           观察 UPackage 作为 UObject 容器的角色
//
// 骨架阶段预期行为: 模块注册成功，StartupModule 打印序列化字节数
//
// 完成后预期日志（Output Log 过滤 LogExE1）:
//   LogExE1: [ExE1] Writer.IsSaving()=true, Reader.IsLoading()=true
//   LogExE1: [ExE1] 序列化字节数: N
//   LogExE1: [ExE1] 往返验证 — Health: 100==100, CharName: Hero==Hero, Speed: 600==600
//   LogExE1: [ExE1] MyObj 所在 Package: /Engine/Transient（临时 Package）
// ============================================================

#include "ExE1_Package.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "UObject/Package.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogExE1, Log, All);

IMPLEMENT_MODULE(FDefaultModuleImpl, ExE1_Package);

// ────────────────────────────────────────────────────────────────────────────
// UExE1Serializable 构造函数
// ────────────────────────────────────────────────────────────────────────────
UExE1Serializable::UExE1Serializable()
{
    UE_LOG(LogExE1, Log, TEXT("[ExE1] UExE1Serializable 构造（CDO 或实例）: %s"),
        *GetNameSafe(this));
}

// ────────────────────────────────────────────────────────────────────────────
// 模块入口 — 演示 FMemoryWriter/FMemoryReader 往返序列化
// ────────────────────────────────────────────────────────────────────────────

// 辅助函数：演示双向序列化流程，可在 StartupModule 或控制台命令里调用
static void RunSerializeDemo()
{
    // ── 1. 创建源对象并赋初值 ────────────────────────────────────────────
    // 注意: NewObject 不指定 Outer 时，对象挂在 GetTransientPackage()
    // 下次 GC 若没有 UPROPERTY 或 AddToRoot 保活，可能被回收
    UExE1Serializable* MyObj = NewObject<UExE1Serializable>();
    MyObj->AddToRoot();  // 临时 AddToRoot 防止 GC 在 demo 期间回收

    MyObj->Health   = 100;
    MyObj->CharName = TEXT("Hero");
    MyObj->Speed    = 600.f;

    // ── 2. FMemoryWriter 序列化（写方向）────────────────────────────────
    TArray<uint8> Buffer;
    FMemoryWriter Writer(Buffer, /*bIsPersistent=*/true);

    // IsSaving() == true 表示当前是写方向
    UE_LOG(LogExE1, Log, TEXT("[ExE1] Writer.IsSaving()=%s, Writer.IsLoading()=%s"),
        Writer.IsSaving()  ? TEXT("true") : TEXT("false"),
        Writer.IsLoading() ? TEXT("true") : TEXT("false"));

    MyObj->Serialize(Writer);  // Tagged Property Serialization
    UE_LOG(LogExE1, Log, TEXT("[ExE1] 序列化字节数: %d"), Buffer.Num());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 创建 Clone，用 FMemoryReader 反序列化，打印字段值对比
    // ══════════════════════════════════════════════════════
    FMemoryReader Reader(Buffer, /*bIsPersistent=*/true);

    UE_LOG(LogExE1, Log, TEXT("[ExE1] Reader.IsSaving()=%s, Reader.IsLoading()=%s"),
        Reader.IsSaving()  ? TEXT("true") : TEXT("false"),
        Reader.IsLoading() ? TEXT("true") : TEXT("false"));

    UExE1Serializable* Clone = NewObject<UExE1Serializable>();
    Clone->AddToRoot();
    Clone->Serialize(Reader);

    UE_LOG(LogExE1, Log,
        TEXT("[ExE1] 往返验证 — Health: %d==%d, CharName: %s==%s, Speed: %.1f==%.1f"),
        MyObj->Health,   Clone->Health,
        *MyObj->CharName, *Clone->CharName,
        MyObj->Speed,    Clone->Speed);

    // ── 3. 观察 UPackage — GetOutermost() 返回所在 Package ──────────────
    UPackage* Pkg = Cast<UPackage>(MyObj->GetOutermost());
    if (Pkg)
    {
        UE_LOG(LogExE1, Log,
            TEXT("[ExE1] MyObj 所在 Package: %s（NewObject 不指定 Outer 则为 TransientPackage）"),
            *Pkg->GetName());
    }

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 重写 UExE1Serializable::Serialize，加入 InternalCounter raw 序列化
    //              观察 Tagged vs Untagged 往返差异
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 2: 用 CreatePackage(TEXT("/Game/ExE1TestPkg")) 建包，
    //              以该包为 Outer 创建对象，再调用 UPackage::SavePackage（Editor only）
    // ══════════════════════════════════════════════════════

    // 清理临时保活
    MyObj->RemoveFromRoot();
    Clone->RemoveFromRoot();
}

// ---- 验证区 ----
// ensureMsgf(Writer.IsSaving() && !Writer.IsLoading(), TEXT("Writer 应处于 Saving 状态"));
// ensureMsgf(!Reader.IsSaving() && Reader.IsLoading(), TEXT("Reader 应处于 Loading 状态"));
// ensureMsgf(Clone->Health == MyObj->Health, TEXT("往返序列化后 Health 应一致"));
