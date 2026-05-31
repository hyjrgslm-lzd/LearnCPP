// ============================================================
// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-2
// C++ 标准要求: C++20
// 本题目标: 理解 CreateDefaultSubobject 只能在构造函数里调用的原因;
//           观察 CDO 创建时机与 Outer 关系;
//           掌握 TObjectPtr access barrier (Editor vs Shipping)
//
// 骨架阶段预期行为: Module 注册, CDO 构造日志
// 完成后预期行为 (PIE 启动日志):
//   LogExD2: UExD2VehicleConfig CDO 构造, EngineSpec 子对象已创建
//   LogExD2: CDO WheelCount=4, CDO EngineSpec=ExD2EngineSpec_0
//   LogExD2: Instance WheelCount=4 (从 CDO 复制默认值)
//   LogExD2: TObjectPtr access barrier: Editor 构建下触发 ConditionallyMarkAsReachable
// ============================================================

#include "ExD2_CDOSubobject.h"
#include "Modules/ModuleManager.h"

// 参考源码:
//   Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1363
//     └── FObjectInitializer::CreateDefaultSubobject (所有重载)
//         注意: CurrentInitializer 在构造函数返回后失效
//   Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:4373
//     └── UClass::GetDefaultObject() —— CDO 懒初始化入口
//   Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h:23
//     └── UE_OBJECT_PTR_GC_BARRIER —— Editor 构建=1, 触发 ConditionallyMarkAsReachable
//         Shipping 构建=0, TObjectPtr<T> 内存布局等价裸 T*, 零开销

IMPLEMENT_MODULE(FDefaultModuleImpl, ExD2_CDOSubobject);

DEFINE_LOG_CATEGORY_STATIC(LogExD2, Log, All);

// ──────────────────────────────────────────────────────────
// UExD2VehicleConfig 构造函数
// ──────────────────────────────────────────────────────────
UExD2VehicleConfig::UExD2VehicleConfig(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    UE_LOG(LogExD2, Log, TEXT("UExD2VehicleConfig 构造: %s"),
        HasAnyFlags(RF_ClassDefaultObject) ? TEXT("CDO") : TEXT("Instance"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 在此创建 EngineSpec 子对象
    //   EngineSpec = ObjectInitializer.CreateDefaultSubobject<UExD2EngineSpec>(
    //       this, TEXT("EngineSpec"));
    //
    // 原因: CreateDefaultSubobject 依赖 FObjectInitializer 的生命周期
    //       FObjectInitializer 在构造函数返回后析构, 其析构函数验证所有 required 子对象已创建
    //       在构造函数外调用 CreateDefaultSubobject 时 CurrentInitializer 为 nullptr,
    //       触发: ensure(CurrentInitializer) 失败 → 崩溃
    // ══════════════════════════════════════════════════════
}

// ══════════════════════════════════════════════════════
// TODO [必做] 2: 在 StartupModule 等入口观察 CDO
// void ObserveCDO()
// {
//     UClass* VehicleClass = UExD2VehicleConfig::StaticClass();
//
//     // CDO: 首次调用 GetDefaultObject 时在 GameThread 上创建
//     UExD2VehicleConfig* CDO = VehicleClass->GetDefaultObject<UExD2VehicleConfig>();
//     UE_LOG(LogExD2, Log, TEXT("CDO WheelCount=%d"), CDO->WheelCount);
//     UE_LOG(LogExD2, Log, TEXT("CDO EngineSpec=%s"), *GetNameSafe(CDO->EngineSpec));
//     UE_LOG(LogExD2, Log, TEXT("CDO Outer=%s"), *GetNameSafe(CDO->GetOuter()));
//     // 预期: CDO Outer 是 Package (通常是 Transient Package)
//
//     // 普通实例: 字段默认值从 CDO 复制而来
//     UExD2VehicleConfig* Instance = NewObject<UExD2VehicleConfig>(GetTransientPackage());
//     UE_LOG(LogExD2, Log, TEXT("Instance WheelCount=%d"), Instance->WheelCount);
//     ensureMsgf(Instance->WheelCount == CDO->WheelCount,
//         TEXT("实例字段默认值应与 CDO 一致"));
//
//     // CDO 与实例的 RF 标志对比
//     ensureMsgf(CDO->HasAnyFlags(RF_ClassDefaultObject),
//         TEXT("CDO 应有 RF_ClassDefaultObject"));
//     ensureMsgf(!Instance->HasAnyFlags(RF_ClassDefaultObject),
//         TEXT("普通实例不应有 RF_ClassDefaultObject"));
// }
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [必做] 3: 在构造函数外调用 CreateDefaultSubobject (观察崩溃)
// void BadSubobjectOutsideCtor(UExD2VehicleConfig* Obj)
// {
//     // 警告: 下面这行会崩溃! FObjectInitializer 已不存在
//     // UExD2EngineSpec* BadSpec = NewObject<UExD2EngineSpec>(Obj, TEXT("BadSpec"));
//     // 正确的运行时动态创建子对象方式:
//     // UExD2EngineSpec* DynSpec = NewObject<UExD2EngineSpec>(Obj, TEXT("DynSpec"));
//     // 注意: NewObject 在构造函数外是合法的, 但它不是 "default subobject",
//     //       不参与蓝图覆写和序列化的 subobject 机制
// }
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [必做] 4: TObjectPtr access barrier 观察
// void ObserveTObjectPtr(UExD2VehicleConfig* CDO)
// {
//     // 在 Editor 构建 (UE_OBJECT_PTR_GC_BARRIER=1) 下:
//     //   读取 TObjectPtr 字段触发 ConditionallyMarkAsReachable
//     //   增量 GC 运行期间, 此操作防止漏标 (write barrier)
//     //   源码: ObjectPtr.h:71-77
//     TObjectPtr<UExD2EngineSpec> Ptr = CDO->EngineSpec;  // 触发 access tracking
//
//     // 在 Shipping 构建 (UE_OBJECT_PTR_GC_BARRIER=0) 下:
//     //   TObjectPtr<T> 内存布局与裸 T* 完全等价, 零开销
//     //   该 ConditionallyMarkAsReachable 调用被条件编译掉
//     UE_LOG(LogExD2, Log, TEXT("TObjectPtr EngineSpec=%s"), *GetNameSafe(Ptr));
// }
// ══════════════════════════════════════════════════════


// ---- 验证区 (完成 TODO 后把下列 ensure 搬入对应代码路径) ----
// UExD2VehicleConfig* CDO = UExD2VehicleConfig::StaticClass()->GetDefaultObject<UExD2VehicleConfig>();
// ensureMsgf(CDO != nullptr, TEXT("CDO 不应为 nullptr"));
// ensureMsgf(CDO->HasAnyFlags(RF_ClassDefaultObject), TEXT("CDO 应有 RF_ClassDefaultObject 标志"));
// ensureMsgf(CDO->EngineSpec != nullptr, TEXT("CDO 的 EngineSpec 子对象应已在构造函数中创建"));
// UE_LOG(LogExD2, Log, TEXT("ExD2 验证通过"));
