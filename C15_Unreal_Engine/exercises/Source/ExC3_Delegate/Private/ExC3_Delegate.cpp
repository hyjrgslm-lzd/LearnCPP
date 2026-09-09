// ============================================================
// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-3
// C++ 标准要求: C++20
// 本题目标: 手写最小 multicast delegate (implement-own);
//           对比 DECLARE_DELEGATE_OneParam / DECLARE_MULTICAST_DELEGATE /
//           DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam 三型差异
//
// 骨架阶段预期行为: Module 注册, FMiniIntDelegate 可实例化
// 完成后预期行为 (PIE 启动日志):
//   LogExC3: Broadcast(42) → H1: 42, H2: 42
//   LogExC3: Remove(H1) 后 Broadcast(7) → 仅 H2: 7
//   LogExC3: UE 单播 Execute: Value=100
//   LogExC3: UE 多播 Broadcast: A=99, B=99
// ============================================================

#include "ExC3_Delegate.h"
#include "Modules/ModuleManager.h"

// 参考源码:
//   Engine/Source/Runtime/Core/Public/Delegates/Delegate.h
//     └── C++ delegate 系统完整文档注释; payload 机制; dynamic delegate 区别
//   Engine/Source/Runtime/Core/Public/Delegates/DelegateCombinations.h
//     └── DECLARE_DELEGATE_OneParam(Name, Param1Type)        → 单播
//     └── DECLARE_MULTICAST_DELEGATE_OneParam(Name, Param1Type) → 多播
//     └── DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(Name, Param1Type, Param1Name) → 反射层
//   Engine/Source/Runtime/Core/Public/Templates/Function.h
//     └── TFunction<void(int32)>: UE 版 std::function, SBO 默认 32 字节

IMPLEMENT_MODULE(FDefaultModuleImpl, ExC3_Delegate);

DEFINE_LOG_CATEGORY_STATIC(LogExC3, Log, All);

// ──────────────────────────────────────────────────────────
// FMiniDelegateHandle 实现
// ──────────────────────────────────────────────────────────

// 线程不安全的静态计数器 (本题限单线程使用)
static uint64 GNextDelegateHandleID = 1;

FMiniDelegateHandle FMiniDelegateHandle::Generate()
{
    FMiniDelegateHandle H;
    H.ID = GNextDelegateHandleID++;
    return H;
}

// ──────────────────────────────────────────────────────────
// FMiniIntDelegate 实现
// ──────────────────────────────────────────────────────────

FMiniDelegateHandle FMiniIntDelegate::AddLambda(TFunction<void(int32)> Lambda)
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 生成句柄, 创建 FMiniBinding, 追加到 Bindings
    //   FMiniBinding Binding;
    //   Binding.Handle = FMiniDelegateHandle::Generate();
    //   Binding.Func   = MoveTemp(Lambda);
    //   Bindings.Add(MoveTemp(Binding));
    //   return Bindings.Last().Handle;
    // ══════════════════════════════════════════════════════
    return FMiniDelegateHandle{};  // 骨架: 返回无效句柄
}

void FMiniIntDelegate::Remove(FMiniDelegateHandle Handle)
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 5: 按 Handle.ID 线性扫描并删除
    //   Bindings.RemoveAll([&Handle](const FMiniBinding& B)
    //   {
    //       return B.Handle == Handle;
    //   });
    // ══════════════════════════════════════════════════════
}

void FMiniIntDelegate::Broadcast(int32 Value)
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 6: 遍历 Bindings 并调用每个 Func
    //   // 注意: 不要在遍历中 Remove (进阶任务才处理 Remove-during-Broadcast)
    //   for (const FMiniBinding& Binding : Bindings)
    //   {
    //       if (Binding.Func)
    //           Binding.Func(Value);
    //   }
    // ══════════════════════════════════════════════════════
}

// ──────────────────────────────────────────────────────────
// FMiniSingleDelegate 实现
// ──────────────────────────────────────────────────────────

void FMiniSingleDelegate::BindLambda(TFunction<void(int32)> Lambda)
{
    // TODO [进阶] 1: BoundFunc = MoveTemp(Lambda); bIsBound = true;
}

void FMiniSingleDelegate::Execute(int32 Value)
{
    // TODO [进阶] 1: check(bIsBound); BoundFunc(Value);
    checkf(bIsBound, TEXT("FMiniSingleDelegate::Execute 调用时未绑定"));
}

void FMiniSingleDelegate::ExecuteIfBound(int32 Value)
{
    // TODO [进阶] 1: if (bIsBound) BoundFunc(Value);
}

bool FMiniSingleDelegate::IsBound() const
{
    return bIsBound;
}

void FMiniSingleDelegate::Unbind()
{
    BoundFunc  = nullptr;
    bIsBound   = false;
}

// ══════════════════════════════════════════════════════
// TODO [必做] 7: 验证函数 (在 StartupModule 等入口调用)
// void RunExC3Validation()
// {
//     FMiniIntDelegate D;
//     FMiniDelegateHandle H1 = D.AddLambda([](int32 X)
//     {
//         UE_LOG(LogExC3, Log, TEXT("H1: %d"), X);
//     });
//     FMiniDelegateHandle H2 = D.AddLambda([](int32 X)
//     {
//         UE_LOG(LogExC3, Log, TEXT("H2: %d"), X);
//     });
//     D.Broadcast(42);   // 期望: H1: 42, H2: 42
//     D.Remove(H1);
//     D.Broadcast(7);    // 期望: 仅 H2: 7
//
//     // UE 宏版本 —— 对比演示
//     FExC3MulticastDelegate UEMulti;
//     UEMulti.AddLambda([](int32 X){ UE_LOG(LogExC3, Log, TEXT("UEMulti A: %d"), X); });
//     UEMulti.AddLambda([](int32 X){ UE_LOG(LogExC3, Log, TEXT("UEMulti B: %d"), X); });
//     UEMulti.Broadcast(99);   // 期望: A: 99, B: 99
//
//     FExC3SingleDelegate UESingle;
//     UESingle.BindLambda([](int32 X){ UE_LOG(LogExC3, Log, TEXT("UESingle: %d"), X); });
//     UESingle.Execute(100);   // 期望: UESingle: 100
// }
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 2: Remove-during-Broadcast 延迟删除
//   class FMiniIntDelegateSafe : public FMiniIntDelegate {
//       TArray<FMiniDelegateHandle> PendingRemove;
//       bool bBroadcasting = false;
//   public:
//       void Remove(FMiniDelegateHandle H) override {
//           if (bBroadcasting) PendingRemove.Add(H);
//           else FMiniIntDelegate::Remove(H);
//       }
//       void Broadcast(int32 V) {
//           bBroadcasting = true;
//           FMiniIntDelegate::Broadcast(V);
//           bBroadcasting = false;
//           for (auto& H : PendingRemove) FMiniIntDelegate::Remove(H);
//           PendingRemove.Reset();
//       }
//   };
// ══════════════════════════════════════════════════════


// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// FMiniIntDelegate TestD;
// FMiniDelegateHandle TH = TestD.AddLambda([](int32){});
// ensureMsgf(TH.IsValid(), TEXT("AddLambda 返回的句柄应有效"));
// ensureMsgf(TestD.IsBound(), TEXT("绑定后 IsBound 应为 true"));
// TestD.Remove(TH);
// ensureMsgf(!TestD.IsBound(), TEXT("Remove 后 IsBound 应为 false"));
// UE_LOG(LogExC3, Log, TEXT("ExC3 验证通过"));
