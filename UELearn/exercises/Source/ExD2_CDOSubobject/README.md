> 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-2

## 目标

理解 `CreateDefaultSubobject` 只能在构造函数里调用的原因；观察 CDO 的创建时机与 Outer 关系；掌握 `TObjectPtr<T>` 的 access barrier 在 Editor 与 Shipping 构建下的行为差异（`UE_OBJECT_PTR_GC_BARRIER`）。

## 前置理解

- 已完成 ExD1_HelloUObject，理解 `GENERATED_BODY()` 展开内容和 UHT 流程。
- 了解 `FObjectInitializer` 是 `UObject` 构造函数的参数，负责跟踪构造期的子对象创建请求。
- 已读 `01-心智模型.md` §4（GC ownership vs C++ ownership 的边界）。

## 必做任务

1. 取消 `UExD2EngineSpec` 和 `UExD2VehicleConfig` 头文件中 `UPROPERTY` 的注释，使 UHT 为其生成反射描述符。
2. 在 `UExD2VehicleConfig` 构造函数中用 `ObjectInitializer.CreateDefaultSubobject<UExD2EngineSpec>(this, TEXT("EngineSpec"))` 创建子对象，赋值给 `EngineSpec`。
3. 在 `StartupModule` 中观察 CDO：调用 `GetDefaultObject<UExD2VehicleConfig>()`，打印 `CDO->WheelCount`、`CDO->EngineSpec` 的名字、CDO 的 Outer。
4. 在构造函数**之外**（普通成员函数里）尝试调用 `CreateDefaultSubobject`，记录崩溃或断言信息，理解 `FObjectInitializer` 生命周期的含义。
5. 观察 `TObjectPtr` access barrier：在 Editor 构建（`UE_OBJECT_PTR_GC_BARRIER=1`）下，读取 `TObjectPtr` 字段触发 `ConditionallyMarkAsReachable`（`ObjectPtr.h:71-77`）；在 Shipping 构建下该调用被条件编译掉。

## 进阶任务

- 取消注释 `UExD2LightVehicle`，在其构造函数里使用 `DoNotCreateDefaultSubobject(TEXT("EngineSpec"))` 阻止父类创建子对象，观察其 CDO 的 `EngineSpec` 是否为 null。
- 查看 `UObjectGlobals.h:1891-1953` 的 `NewObject<T>` 函数模板，对比 CDO 创建路径（`RF_ClassDefaultObject` flag）与普通 `NewObject` 路径的区别。
- 实验 `RF_ClassDefaultObject` 标志：`CDO->HasAnyFlags(RF_ClassDefaultObject)` 与普通实例对比。

## 验收点

- [ ] 编译通过，CDO 构造日志在 PIE Output Log 可见。
- [ ] 能解释 `CreateDefaultSubobject` 为什么只能在构造函数里调用（`FObjectInitializer` 生命周期绑定到对象构造期）。
- [ ] 能说出 CDO 的创建时机（第一次调用 `UClass::GetDefaultObject(true)` 时，`Class.h:4373`）。
- [ ] 能对照源码说出 `TObjectPtr` 在 Editor（`UE_OBJECT_PTR_GC_BARRIER=1`）与 Shipping（`UE_OBJECT_PTR_GC_BARRIER=0`）的行为差异。
- [ ] 能答出四个固定问题。

## 观察点

**TObjectPtr access barrier（UE 特有机制，必须显式理解）**：

`ObjectPtr.h:71-77` 中，`FObjectPtr` 构造函数在 `UE_OBJECT_PTR_GC_BARRIER=1` 时调用 `ConditionallyMarkAsReachable(Object)`。这是增量 GC 的 write barrier：若增量 GC 正在运行，新建的 `TObjectPtr` 会立即把目标对象标记为可达，防止漏标。

Shipping 构建中 `UE_OBJECT_PTR_GC_BARRIER=0`，`TObjectPtr<T>` 内存布局与裸 `T*` 完全等价，零开销。

**两套所有权对照**：
- `UPROPERTY() TObjectPtr<UExD2EngineSpec>` → GC 可见，GC 管理生命周期（推荐）
- `UExD2EngineSpec* EngineSpec`（无 UPROPERTY）→ GC 不可见，随时可能被回收 → 悬空指针

## 常见坑

- **在构造函数外调用 `CreateDefaultSubobject`**：崩溃信息是 `ensure(CurrentInitializer)` 失败。运行时动态添加子对象应使用 `NewObject<T>(this, ...)` 并手动管理。
- **混用 `TSharedPtr<UObject>`**：`TSharedPtr` 引用计数归零触发析构，但 GC 已认为该 UObject 不可达，产生双重析构。绝对禁止。
- **Blueprint 派生类 CDO 与 C++ CDO 的覆写差异**：Blueprint 子类的 CDO 是独立实例，与 C++ CDO 不同，`GetDefaultObject<UCppClass>()` 不会反映 Blueprint 覆写。

## 提示

- `GetNameSafe(CDO->GetOuter())` 可以查看 CDO 的 Outer 是哪个 Package。
- `UObject::GetClass()->GetName()` 可区分对象属于 C++ 类还是 Blueprint 类（Blueprint 类名包含 `_C` 后缀）。
- `HasAnyFlags(RF_ClassDefaultObject)` 是判断 CDO 的标准方式（UE5 中 RootSet 用 `IsRooted()` 而非 `RF_RootSet`）。

## 复盘问题

1. 真正开始执行的时刻？（CDO 在第一次调用 `UClass::GetDefaultObject(true)` 时在 GameThread 上触发；Module 加载时若有代码提前访问 `StaticClass()` 也可能触发。）
2. 谁负责这对象的生命周期？（GC 管理；只要实例被 `UPROPERTY` 引用或 `AddToRoot()` 保护就不会被回收；否则下次 `CollectGarbage` 时被清扫。）
3. 涉及哪些命名线程？（`CreateDefaultSubobject` 和 `NewObject` 均在 GameThread；UObject 系统是单线程的，包括 CDO 构造。）
4. 这对 GC 如何可见？（`TObjectPtr<UExD2EngineSpec> EngineSpec` 带 `UPROPERTY`，GC 通过 `PropertyLink`/`RefLink` 发现此引用；差别在 Editor 下 `TObjectPtr` 触发 GC barrier，裸 `UObject*` 不触发。）
5. [本题专属] `DoNotCreateDefaultSubobject(TEXT("EngineSpec"))` 的内部机制是什么？它在 `FObjectInitializer` 的哪个数据结构中记录"不创建"的意图？

## 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1363`（`FObjectInitializer::CreateDefaultSubobject` 所有重载、`DoNotCreateDefaultSubobject`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1891`（`NewObject<T>` 三个模板重载）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h:23`（`UE_OBJECT_PTR_GC_BARRIER` 定义）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:4373`（`UClass::GetDefaultObject()`）
