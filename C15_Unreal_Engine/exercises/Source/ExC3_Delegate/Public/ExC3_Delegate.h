// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-3
// 小节: 手写最小 multicast delegate (implement-own)
// 三型对比: DECLARE_DELEGATE_OneParam / DECLARE_MULTICAST_DELEGATE / DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam
#pragma once

#include "CoreMinimal.h"
// Delegate.h 包含 C++ delegate 系统的完整文档注释
#include "Delegates/Delegate.h"
// DelegateCombinations.h 定义 DECLARE_*_DELEGATE_* 宏族
#include "Delegates/DelegateCombinations.h"

// ══════════════════════════════════════════════════════
// UE 宏声明三种委托类型 (作为对比参照)
// ══════════════════════════════════════════════════════

// 类型 1: 单播委托 (纯 C++, 零反射)
// 参考: Engine/Source/Runtime/Core/Public/Delegates/DelegateCombinations.h:48
DECLARE_DELEGATE_OneParam(FExC3SingleDelegate, int32);

// 类型 2: 多播委托 (纯 C++, 零反射)
// 参考: Engine/Source/Runtime/Core/Public/Delegates/DelegateCombinations.h:49
DECLARE_MULTICAST_DELEGATE_OneParam(FExC3MulticastDelegate, int32);

// 类型 3: Dynamic 多播委托 (需要 CoreUObject + UHT, 可被蓝图绑定)
// 注意: DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam 展开依赖 UHT 生成的 CURRENT_FILE_ID,
//       因此所在头文件必须有 .generated.h (即必须含 UCLASS/USTRUCT/UENUM).
//       本模块无 UObject 类型, 无法生成 .generated.h, 故在此仅用注释保留参考。
// 参考: Engine/Source/Runtime/Core/Public/Delegates/DelegateCombinations.h:53
// 进阶任务: 把此委托移入一个 UCLASS 的 UPROPERTY, 演示蓝图可见性
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExC3DynamicMulticast, int32, Value);

// ══════════════════════════════════════════════════════
// mini 实现: 句柄类型
// TODO [必做] 1: 实现 FMiniDelegateHandle, ID==0 表示无效句柄
// ══════════════════════════════════════════════════════
struct FMiniDelegateHandle
{
    uint64 ID = 0;

    bool IsValid() const { return ID != 0; }

    bool operator==(const FMiniDelegateHandle& Other) const { return ID == Other.ID; }
    bool operator!=(const FMiniDelegateHandle& Other) const { return ID != Other.ID; }

    // 静态计数器生成唯一 ID (从 1 开始, 0 保留为无效值)
    static FMiniDelegateHandle Generate();
};

// ══════════════════════════════════════════════════════
// mini 实现: 绑定条目
// TODO [必做] 2: 每个 FMiniBinding 持有句柄 + TFunction 函数对象
// ══════════════════════════════════════════════════════
struct FMiniBinding
{
    FMiniDelegateHandle  Handle;
    TFunction<void(int32)> Func;  // TFunction 类似 std::function, 含 SBO 优化
};

// ══════════════════════════════════════════════════════
// mini 实现: 最小多播委托
// TODO [必做] 3-6: 实现 AddLambda / Remove / Broadcast
// ══════════════════════════════════════════════════════
class FMiniIntDelegate
{
public:
    // 注册 lambda, 返回句柄 (用于后续 Remove)
    FMiniDelegateHandle AddLambda(TFunction<void(int32)> Lambda);

    // 按句柄注销绑定 (幂等: 句柄不存在时静默成功)
    void Remove(FMiniDelegateHandle Handle);

    // 广播: 同步调用所有已注册的函数
    // 注意: 骨架阶段不处理 Broadcast 期间的 Remove (进阶任务)
    void Broadcast(int32 Value);

    bool IsBound() const { return Bindings.Num() > 0; }

private:
    TArray<FMiniBinding> Bindings;
};

// ══════════════════════════════════════════════════════
// 进阶: 最小单播委托
// TODO [进阶] 1: 实现 BindLambda / Execute / ExecuteIfBound / IsBound
// ══════════════════════════════════════════════════════
class FMiniSingleDelegate
{
public:
    void BindLambda(TFunction<void(int32)> Lambda);
    void Execute(int32 Value);       // 未绑定时 check() 失败
    void ExecuteIfBound(int32 Value);
    bool IsBound() const;
    void Unbind();

private:
    TFunction<void(int32)> BoundFunc;
    bool bIsBound = false;
};
