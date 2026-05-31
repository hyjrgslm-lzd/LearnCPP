// ============================================================
// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-3
// C++ 标准要求: C++20
// 本题目标: 用 TFieldIterator<FProperty> 遍历 UClass::PropertyLink 链;
//           按 FProperty 子类型分发 (CastField<T>);
//           通过 FObjectProperty 反射读写字段值
//
// 骨架阶段预期行为: Module 注册
// 完成后预期行为 (PIE 启动日志):
//   LogExD3: === DumpProperties: UExD3ReflectTarget ===
//   LogExD3:   [bool]    bActive  offset=XX
//   LogExD3:   [int32]   Count    offset=XX
//   LogExD3:   [float]   Weight   offset=XX
//   LogExD3:   [FString] Label    offset=XX
//   LogExD3:   [TObjectPtr<UObject>] ChildRef  offset=XX
//   LogExD3:   [TArray<...>]   Numbers  offset=XX
//   LogExD3: HiddenField 未出现 (无 UPROPERTY → PropertyLink 中不存在)
//   LogExD3: 反射写入 ChildRef 成功, 读取验证通过
// ============================================================

#include "ExD3_Reflection.h"
#include "Modules/ModuleManager.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"

// 参考源码:
//   Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:211
//     └── FProperty::PropertyLinkNext (链表 next 指针)
//   Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:2542
//     └── FBoolProperty
//   Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:3086
//     └── FObjectProperty (含 PropertyClass 字段)
//   Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:3701
//     └── FArrayProperty (含 Inner 字段)
//   Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:529
//     └── UStruct::PropertyLink (所有 UPROPERTY 的链表)
//     └── UStruct::RefLink (仅含 UObject 引用的属性 —— GC 遍历此链)
//   注意: CastField<T> 用于 FField 体系 (FProperty 族)
//         Cast<T>      用于 UObject 体系
//         两者不可互换! FProperty 从 UE4.25 起不再继承 UObject

IMPLEMENT_MODULE(FDefaultModuleImpl, ExD3_Reflection);

DEFINE_LOG_CATEGORY_STATIC(LogExD3, Log, All);

// ──────────────────────────────────────────────────────────
// 方式 1: 手动遍历 PropertyLink 链
// ──────────────────────────────────────────────────────────
void ExD3_DumpClassPropertiesManual(UClass* Class)
{
    if (!Class) return;

    UE_LOG(LogExD3, Log, TEXT("=== DumpClassProperties (Manual PropertyLink): %s ==="),
        *Class->GetName());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 遍历 PropertyLink 链并按类型分发
    //
    // for (FProperty* Prop = Class->PropertyLink; Prop; Prop = Prop->PropertyLinkNext)
    // {
    //     FString TypeName;
    //
    //     if (CastField<FBoolProperty>(Prop))
    //         TypeName = TEXT("bool");
    //     else if (CastField<FIntProperty>(Prop))
    //         TypeName = TEXT("int32");
    //     else if (CastField<FFloatProperty>(Prop))
    //         TypeName = TEXT("float");
    //     else if (CastField<FStrProperty>(Prop))
    //         TypeName = TEXT("FString");
    //     else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
    //         TypeName = FString::Printf(TEXT("TObjectPtr<%s>"), *ObjProp->PropertyClass->GetName());
    //     else if (CastField<FArrayProperty>(Prop))
    //         TypeName = TEXT("TArray<...>");
    //     else
    //         TypeName = Prop->GetClass()->GetName();
    //
    //     UE_LOG(LogExD3, Log, TEXT("  [%s] Name=%s Offset=%d Flags=0x%llX"),
    //         *TypeName,
    //         *Prop->GetName(),
    //         Prop->GetOffset_ForInternal(),
    //         (uint64)Prop->PropertyFlags);
    // }
    //
    // 观察: HiddenField 不出现 —— UHT 未为其生成 FProperty 描述符
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExD3, Log, TEXT("请完成 TODO [必做] 2: 遍历 PropertyLink"));
}

// ──────────────────────────────────────────────────────────
// 方式 2: TFieldIterator (UE 推荐方式)
// ──────────────────────────────────────────────────────────
void ExD3_DumpClassPropertiesIterator(UClass* Class)
{
    if (!Class) return;

    UE_LOG(LogExD3, Log, TEXT("=== DumpClassProperties (TFieldIterator): %s ==="),
        *Class->GetName());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 4: 用 TFieldIterator 改写
    //
    // // EFieldIteratorFlags::IncludeSuper: 包含继承自基类的属性
    // // EFieldIteratorFlags::ExcludeSuper: 只遍历本类定义的属性
    // for (TFieldIterator<FProperty> PropIt(Class); PropIt; ++PropIt)
    // {
    //     FProperty* Prop = *PropIt;
    //     UE_LOG(LogExD3, Log, TEXT("  TFieldIterator: %s (%s) offset=%d"),
    //         *Prop->GetName(),
    //         *Prop->GetCPPType(),
    //         Prop->GetOffset_ForInternal());
    // }
    //
    // 对比手动遍历 PropertyLink vs TFieldIterator:
    //   手动遍历: 从最派生类到基类的 PropertyLink 链, 需自行处理继承层次
    //   TFieldIterator: 自动遍历整个继承链, 可用 EFieldIteratorFlags 控制深度
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExD3, Log, TEXT("请完成 TODO [必做] 4: 使用 TFieldIterator"));
}

// ──────────────────────────────────────────────────────────
// 通过反射写入并验证 ChildRef 字段
// ──────────────────────────────────────────────────────────
void ExD3_WriteAndReadViaReflection()
{
    // ══════════════════════════════════════════════════════
    // TODO [必做] 5: 通过 FObjectProperty 反射写入 ChildRef
    //
    // UExD3ReflectTarget* Target = NewObject<UExD3ReflectTarget>(GetTransientPackage());
    //
    // // FindPropertyByName 在 PropertyLink 链中按名字查找
    // FObjectProperty* ChildRefProp = CastField<FObjectProperty>(
    //     UExD3ReflectTarget::StaticClass()->FindPropertyByName(TEXT("ChildRef")));
    // ensureMsgf(ChildRefProp != nullptr, TEXT("ChildRef 属性应存在 (需先取消 UPROPERTY 注释)"));
    //
    // UObject* NewChild = NewObject<UObject>(Target);
    //
    // // SetObjectPropertyValue_InContainer: 写入偏移处的指针值
    // // GetOffset_ForInternal() 即字段相对对象起始的字节偏移
    // ChildRefProp->SetObjectPropertyValue_InContainer(Target, NewChild);
    //
    // UObject* Retrieved = ChildRefProp->GetObjectPropertyValue_InContainer(Target);
    // ensureMsgf(Retrieved == NewChild, TEXT("反射写入后应能正确读取"));
    // UE_LOG(LogExD3, Log, TEXT("反射写入 ChildRef 成功: %s"), *GetNameSafe(Retrieved));
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExD3, Log, TEXT("请完成 TODO [必做] 5: 反射写入 ChildRef"));
}

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 遍历 UObject 基类的 PropertyLink
// void InspectUObjectBaseProperties()
// {
//     UE_LOG(LogExD3, Log, TEXT("UObject 基类 UPROPERTY 数量:"));
//     int32 Count = 0;
//     for (TFieldIterator<FProperty> It(UObject::StaticClass(),
//             EFieldIterationFlags::ExcludeSuper); It; ++It)
//         ++Count;
//     UE_LOG(LogExD3, Log, TEXT("  UObject 自身定义的 UPROPERTY 数 = %d"), Count);
//     // 通常为 0 —— UObject 的大多数基础字段不暴露给反射
// }
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 2: 通过 FindFunctionByName + ProcessEvent 调用 UFUNCTION
// void CallFunctionViaReflection(UExD3ReflectTarget* Target)
// {
//     UFunction* Func = UExD3ReflectTarget::StaticClass()
//         ->FindFunctionByName(TEXT("OnPropertyChanged"));
//     if (Func && Target)
//         Target->ProcessEvent(Func, nullptr);
// }
// ══════════════════════════════════════════════════════


// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// // 验证 HiddenField 不在 PropertyLink 中
// FProperty* HiddenProp = UExD3ReflectTarget::StaticClass()
//     ->FindPropertyByName(TEXT("HiddenField"));
// ensureMsgf(HiddenProp == nullptr,
//     TEXT("无 UPROPERTY 的字段不应出现在 PropertyLink 中"));
// UE_LOG(LogExD3, Log, TEXT("ExD3 验证通过"));
