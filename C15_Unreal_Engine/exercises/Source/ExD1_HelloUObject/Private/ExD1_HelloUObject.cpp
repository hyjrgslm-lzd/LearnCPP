// ============================================================
// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-1
// C++ 标准要求: C++20
// 本题目标: 写出第一个 UCLASS; 观察 UHT 代码生成;
//           逐段读懂 .generated.h; 理解 CDO 构造时机
//
// 骨架阶段预期行为: Module 注册, CDO 构造日志
// 完成后预期行为 (PIE 启动日志):
//   LogExD1: ExD1 CDO 构造: UExD1SampleObject (RF_ClassDefaultObject)
//   LogExD1: ExD1 实例构造: Counter=0
//   LogExD1: AdvanceCounter -> Counter=1
//   LogExD1: StaticClass == GetClass: true
// ============================================================

#include "ExD1_HelloUObject.h"
#include "Modules/ModuleManager.h"

// 参考源码:
//   Engine/Source/Runtime/CoreUObject/Public/UObject/Object.h:93
//     └── UObject 基类: UCLASS(Abstract, MinimalAPI) + GENERATED_BODY()
//   Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h:764
//     └── GENERATED_BODY() = BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_GENERATED_BODY)
//         展开三块: INCLASS_NO_PURE_DECLS / ENHANCED_CONSTRUCTORS / CALLBACK_WRAPPERS
//   Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1889
//     └── NewObject<T>() → FStaticConstructObjectParameters → StaticConstructObject_Internal

IMPLEMENT_MODULE(FDefaultModuleImpl, ExD1_HelloUObject);

DEFINE_LOG_CATEGORY_STATIC(LogExD1, Log, All);

// ──────────────────────────────────────────────────────────
// UExD1SampleObject 构造函数
// ──────────────────────────────────────────────────────────
UExD1SampleObject::UExD1SampleObject()
{
    // HasAnyFlags(RF_ClassDefaultObject) 为 true 时, 当前对象是 CDO
    // CDO 在 UClass::GetDefaultObject(true) 首次调用时创建 (GameThread, Module 加载阶段)
    // 参考: Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:4373
    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        UE_LOG(LogExD1, Log, TEXT("ExD1 CDO 构造: %s (RF_ClassDefaultObject)"),
            *GetClass()->GetName());
    }
    else
    {
        UE_LOG(LogExD1, Log, TEXT("ExD1 实例构造: %s"), *GetNameSafe(this));
    }

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 初始化 Counter
    //   // 只在非 CDO 时赋初值, 演示 CDO 与实例的区别
    //   if (!HasAnyFlags(RF_ClassDefaultObject))
    //   {
    //       Counter = 0;
    //   }
    // ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// TODO [必做] 2: 实现 AdvanceCounter
// void UExD1SampleObject::AdvanceCounter()
// {
//     ++Counter;
//     UE_LOG(LogExD1, Log, TEXT("AdvanceCounter -> Counter=%d"), Counter);
// }
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 实现 DumpProperties (用 TFieldIterator 遍历所有 UPROPERTY)
// void UExD1SampleObject::DumpProperties()
// {
//     UE_LOG(LogExD1, Log, TEXT("=== DumpProperties: %s ==="), *GetClass()->GetName());
//     for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
//     {
//         FProperty* Prop = *It;
//         UE_LOG(LogExD1, Log, TEXT("  [%s] %s  offset=%d"),
//             *Prop->GetCPPType(),
//             *Prop->GetName(),
//             Prop->GetOffset_ForInternal());
//     }
// }
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [必做] 3: 在 StartupModule 等入口调用验证代码
// void RunD1Validation()
// {
//     // 创建实例 (必须用 NewObject, 不能裸 new)
//     // 参考: ObjectMacros.h "坑 3": 裸 new 创建的 UObject 不在 GUObjectArray 中
//     UExD1SampleObject* Obj = NewObject<UExD1SampleObject>(GetTransientPackage());
//
//     // 验证 StaticClass() 与 GetClass() 返回同一个 UClass*
//     // 原因: StaticClass() 是类级别的全局单例; GetClass() 读取实例的 ClassPrivate 字段
//     //       两者指向同一个 UClass 对象
//     ensureMsgf(UExD1SampleObject::StaticClass() == Obj->GetClass(),
//         TEXT("StaticClass 与 GetClass 应返回相同的 UClass*"));
//
//     // 验证 CDO 有 RF_ClassDefaultObject 标志
//     UExD1SampleObject* CDO = UExD1SampleObject::StaticClass()->GetDefaultObject<UExD1SampleObject>();
//     ensureMsgf(CDO->HasAnyFlags(RF_ClassDefaultObject), TEXT("CDO 应有 RF_ClassDefaultObject"));
//     ensureMsgf(!Obj->HasAnyFlags(RF_ClassDefaultObject), TEXT("普通实例不应有 RF_ClassDefaultObject"));
// }
// ══════════════════════════════════════════════════════


// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// ensureMsgf(Obj->Counter == 0, TEXT("新创建实例 Counter 应为 0"));
// Obj->AdvanceCounter();
// ensureMsgf(Obj->Counter == 1, TEXT("AdvanceCounter 后 Counter 应为 1"));
// ensureMsgf(GetNameSafe(Obj).Len() > 0, TEXT("对象名不应为空"));
// UE_LOG(LogExD1, Log, TEXT("ExD1 验证通过"));
