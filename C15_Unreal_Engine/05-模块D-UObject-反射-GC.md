# 05 模块 D：UObject / 反射 / GC（枢纽）

## 模块目标

反向引用 `01-心智模型` §2（Reflection before types）和 §4（GC ownership vs C++ ownership 两套并存）。

本模块是整套课程的枢纽。UE 世界里几乎所有可见对象——Actor、Component、资产、蓝图——都建立在 UObject / 反射 / GC 这三层机制之上。跳过或浅过本模块，后续模块的每一个 API 细节都会变成没有根基的记忆碎片。

本模块将带你走完四级阶梯：**use → inspect → implement-own**。

**阶梯定位**：D1 use / D2 use + inspect / D3 inspect / D4 implement-own（核心输出）。

做完本模块，你至少要能稳定说清楚：

1. UHT 在 C++ 编译器之前跑，`.generated.h` 由 UHT 产生，`GENERATED_BODY()` 展开为哪几类声明。
2. CDO（Class Default Object）何时创建，`CreateDefaultSubobject` 只能在构造函数里调用的原因。
3. UE 的 `PropertyLink` 链是什么，如何用 `TFieldIterator` 遍历一个类的所有 `UPROPERTY` 字段。
4. UE 的 mark-sweep GC 如何从 RootSet 出发，通过 `PropertyLink` 递归标记可达对象，并清扫未标记对象。
5. **两套所有权的绝对分界**：`UPROPERTY` / `TObjectPtr` / `AddToRoot` 让 GC 可见；`TSharedPtr` / 裸指针（非 UPROPERTY）属 C++ 所有权空间，GC 完全不可见。
6. `TObjectPtr` 的 access barrier 在 Editor 构建中开启，在 Shipping 中退化为裸指针——这是 UE5 的关键 ABI 差异。

---

## D1 — Hello UObject + 亲手打开 .generated.h

### 目标

写出第一个 `UCLASS`，观察 UHT 代码生成机制，逐段读懂 `.generated.h`，把"UHT 在 C++ 编译器之前跑"从抽象概念变成眼见为实的体验。

**UE 特有点（必读）**：这是整套课程**第一次直面代码生成**。UHT 不是 C++ 编译器的一部分，它是一个在编译器之前独立运行的 Parser，输出是纯 C++ 文件。理解这个顺序是理解 UE 一切宏的前提。

### 前置理解

- 你已经读过 `01-心智模型` §2（Reflection before types），知道 `.generated.h` 的必要性。
- 你知道 `#include "MyClass.generated.h"` 必须是头文件的最后一个 `#include`，原因是 UHT 对 include 顺序有硬性要求。
- 你了解模块 A 的基础——已经搭好一个可被 UE 加载的 Module，本题在此基础上写 UCLASS。

### 必做任务

1. 在你的练习 Module（建议命名 `D1_HelloUObject`）中创建头文件 `MyDataAsset.h`，声明一个简单的 `UCLASS`：
   ```cpp
   // MyDataAsset.h
   #pragma once
   #include "UObject/Object.h"
   #include "MyDataAsset.generated.h"   // 必须最后一个 include

   UCLASS(Blueprintable, BlueprintType)
   class D1HELLOUOBJECT_API UMyDataAsset : public UObject
   {
       GENERATED_BODY()
   public:
       UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
       int32 Health = 100;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
       float Speed = 300.f;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
       FString DisplayName = TEXT("Default");

       UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Demo")
       TObjectPtr<UObject> OwnerRef;  // TObjectPtr 而非裸 UObject*

       UFUNCTION(BlueprintCallable, Category = "Demo")
       void PrintInfo() const;
   };
   ```

2. 编译（触发 UHT 先跑）。找到生成目录下该类对应的 `.generated.h`（路径形如 `Engine/Intermediate/Build/Win64/UnrealEditor/Inc/<Module>/UHT/MyDataAsset.generated.h`），用文本编辑器打开。

3. **逐段分析 `.generated.h` 展开内容**，在你的观察记录里回答下列每一点：
   - `GENERATED_BODY()` 宏展开后具体替换为哪个宏？（对照 `Object.generated.h` 的结构：`FID_..._GENERATED_BODY`，它拼接了三块：`CALLBACK_WRAPPERS` / `INCLASS_NO_PURE_DECLS` / `ENHANCED_CONSTRUCTORS`。）
   - `INCLASS_NO_PURE_DECLS` 里有什么？（关键：`static void StaticRegisterNativesXxx()`、`GetPrivateStaticClass()`、`DECLARE_CLASS2` 宏展开的 `StaticClass()` 声明。）
   - `ENHANCED_CONSTRUCTORS` 里有什么？（删除的拷贝/移动构造、`DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER`、`DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL`。）
   - `Z_Construct_UClass_UMyDataAsset_NoRegister()` 函数声明的作用？（它是反射元数据的懒初始化入口，首次调用时注册 UClass 到 GUObjectArray。）
   - `.gen.cpp` 文件（同目录）里的 `Z_Construct_UClass_UMyDataAsset_Statics` 结构体中包含哪些内容？（PropPointers 数组、FuncInfo 数组、MetaData 字符串常量。）

4. 用 `NewObject<UMyDataAsset>()` 在 `StartupModule` 里创建一个实例，并调用 `PrintInfo()`，验证对象正常工作。

5. 添加 `UCLASS(meta=(ToolTip="这是一个演示数据资产"))` 观察生成的 `gen.cpp` 里 meta 数据的格式变化。

### 进阶任务

- 在 `UMyDataAsset` 上添加 `UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))` 等不同 meta 键值，每次修改后重新 UHT，观察生成文件的差异——这是 UE 的 Mini-DSL meta 系统（`01-心智模型` §10.5 提到的"非 C++ attribute"）。
- 对比 `UCLASS(Abstract)` vs 无 Abstract：前者的 `GetPrivateStaticClass()` 调用 `DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL`，后者调用 `DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL`，它们影响 CDO 能否直接被实例化。
- 故意把 `#include "MyDataAsset.generated.h"` 从头文件最后一行移到第一行，记录 UHT 报错 vs C++ 编译器报错的区别。

### 验收点

- 你能不借助文档，用一段话解释 `.generated.h` 里三个 `#define` 块各自的职责。
- 你能在源码中定位 `DECLARE_CLASS2` 宏（位于 `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h`），说明它展开了哪些 `StaticClass()`、`GetPrivateStaticClass()` 相关内容。
- 你能说出为什么"改了 UCLASS/UPROPERTY 的头文件但没有重新跑 UHT"会导致一类特殊的编译错误，而不是普通的 C++ 编译错误。
- 创建的对象调用 `UMyDataAsset::StaticClass()` 返回的 `UClass*` 和 `GetClass()` 返回的是否相同对象？为什么？

### 观察点

- UHT 的报错和 C++ 编译器的报错在 IDE 输出里来自不同的进程，错误格式也不同：UHT 错误会显示"UnrealHeaderTool"字样，而 C++ 错误来自 CL.exe / Clang。
- `Z_Construct_UClass_UMyDataAsset_NoRegister()` 使用了惰性初始化模式——只有第一次被调用时才真正执行注册，这与 UE 的模块按需加载设计一致。
- `UPROPERTY(TObjectPtr<UObject> OwnerRef)` 与裸 `UObject* OwnerRef` 在生成的 `.gen.cpp` 里的 `FObjectPropertyDesc` 完全相同——`TObjectPtr` 的差别体现在运行时（access barrier），不在 UHT 元数据层。
- 源码参考：
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\Object.h:93-96`（`UCLASS(Abstract, MinimalAPI)` + `GENERATED_BODY()` 示例）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Intermediate\Build\Win64\UnrealEditor\Inc\CoreUObject\UHT\Object.generated.h:21-64`（生成文件结构完整样本：`INCLASS_NO_PURE_DECLS` / `ENHANCED_CONSTRUCTORS` / `GENERATED_BODY` 三块拼接）

### 常见坑 / 提示

- **坑 1**：`#include "MyClass.generated.h"` 放错位置。UHT 要求它是最后一个 include，放在前面时 UHT 会报错 `'...generated.h' must be the last include`，这个错误来自 UHT 阶段，与编译器无关。
- **坑 2**：模块 API 宏（`D1HELLOUOBJECT_API`）拼写错误或模块名包含特殊字符。UBT 生成的 API 宏规则：模块名转大写 + `_API`，含连字符的模块名会被转换。
- **坑 3**：直接在磁盘上 `new UMyDataAsset()` 而不是 `NewObject<UMyDataAsset>()`。裸 `new` 创建的 UObject 不在 `GUObjectArray` 中，不受 GC 管理，绝大多数 UE 框架函数访问它时会断言失败。
- **提示**：生成文件路径依赖你的工程目录和模块名，一般在 `<Project>/Intermediate/Build/<Platform>/<Target>/Inc/<ModuleName>/UHT/`。

### 复盘问题

- **Q1（执行真正开始时刻）**：`UMyDataAsset::StaticClass()` 的元数据第一次注册发生在什么时刻？（答：Module 的 `StartupModule` 调用后，`Z_Construct_UClass_UMyDataAsset` 被 `FClassRegistrationInfo` 的惰性初始化触发，发生在 GameThread，Module 加载阶段。）
- **Q2（生命周期拥有者）**：用 `NewObject<UMyDataAsset>(GetTransientPackage())` 创建的对象，谁拥有它的生命周期？（答：GC 管理，Outer 是 Transient Package；只要没有 `UPROPERTY` 引用它且没有 `AddToRoot()`，下次 GC 即可回收。）
- **Q3（Named Thread）**：`NewObject<UMyDataAsset>()` 只能在哪条线程调用？（答：只能在 GameThread。UObject 分配、反射查找、GUObjectArray 写入都假设在 GameThread，Worker thread 上调用会触发断言。）
- **Q4（GC 可见性）**：`OwnerRef` 字段用 `TObjectPtr<UObject>` 且有 `UPROPERTY` 标注，GC 如何发现它？（答：UHT 在 `.gen.cpp` 里生成 `FObjectProperty` 描述符，链入 `UClass::PropertyLink`；GC 遍历时通过 `PropertyLink` 找到 `OwnerRef` 字段的偏移，读取指针值并标记目标对象为可达。）
- **UHT 与编译器顺序**：为什么 UE 选择在 C++ 编译器之前跑 UHT，而不是用 C++ 模板实现运行时反射？（答：模板反射需要完整的类型信息在编译期，且生成的元数据不能被非 C++ 工具（蓝图 VM、序列化器）直接消费；UHT 产出的是纯数据结构，语言无关，这是设计权衡。）

### 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/Object.h:93`（`UObject` 基类 `UCLASS` + `GENERATED_BODY()`）
- `Engine/Intermediate/Build/Win64/UnrealEditor/Inc/CoreUObject/UHT/Object.generated.h:21-64`（展开结构样本）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h`（`DECLARE_CLASS2`、`GENERATED_BODY` 等宏定义）
- UE 官方文档：Objects | Unreal Engine 5 Documentation

---

## D2 — CreateDefaultSubobject 与构造阶段 / CDO

### 目标

理解 `CreateDefaultSubobject` 只能在构造函数里调用的原因，理解 CDO（Class Default Object）是什么以及 `NewObject` vs 构造期的区别。掌握 `TObjectPtr` 的 access barrier 在 Editor 与 Shipping 下的行为差异（**UE 特有机制 §10.4**）。

### 前置理解

- 你已经完成 D1，理解 `GENERATED_BODY()` 展开内容和 UHT 流程。
- 你了解 `FObjectInitializer` 是什么——它是 `UObject` 构造函数的参数，负责跟踪构造期的子对象创建请求。
- 你从 `01-心智模型` §4 了解 GC ownership 与 C++ ownership 的边界。

### 必做任务

1. 创建一个新的 Module `D2_CDO`，定义一个 UObject 子类 `UVehicleConfig`，在构造函数里使用 `CreateDefaultSubobject`：
   ```cpp
   // UVehicleConfig.h
   #pragma once
   #include "UObject/Object.h"
   #include "UVehicleConfig.generated.h"

   UCLASS()
   class D2CDO_API UEngineSpec : public UObject
   {
       GENERATED_BODY()
   public:
       UPROPERTY(EditAnywhere)
       float Horsepower = 400.f;
   };

   UCLASS()
   class D2CDO_API UVehicleConfig : public UObject
   {
       GENERATED_BODY()
   public:
       UVehicleConfig(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

       // TObjectPtr 是 UE5 推荐的 UPROPERTY 成员类型
       UPROPERTY(EditAnywhere)
       TObjectPtr<UEngineSpec> EngineSpec;

       UPROPERTY(EditAnywhere)
       int32 WheelCount = 4;
   };
   ```
   ```cpp
   // UVehicleConfig.cpp
   UVehicleConfig::UVehicleConfig(const FObjectInitializer& ObjectInitializer)
       : Super(ObjectInitializer)
   {
       // CreateDefaultSubobject 只能在构造函数里调用！
       EngineSpec = ObjectInitializer.CreateDefaultSubobject<UEngineSpec>(this, TEXT("EngineSpec"));
   }
   ```

2. **观察 CDO**：在 `StartupModule` 里写：
   ```cpp
   UClass* VehicleClass = UVehicleConfig::StaticClass();
   UVehicleConfig* CDO = VehicleClass->GetDefaultObject<UVehicleConfig>();
   UE_LOG(LogTemp, Warning, TEXT("CDO WheelCount = %d"), CDO->WheelCount);
   UE_LOG(LogTemp, Warning, TEXT("CDO EngineSpec = %s"), *GetNameSafe(CDO->EngineSpec));
   
   // 创建一个普通实例并对比
   UVehicleConfig* Instance = NewObject<UVehicleConfig>(GetTransientPackage());
   UE_LOG(LogTemp, Warning, TEXT("Instance WheelCount = %d"), Instance->WheelCount);
   ```
   记录：CDO 是何时创建的？CDO 的 Outer 是什么对象？实例的字段值从 CDO 复制而来吗？

3. **`FObjectInitializer` 与 `DoNotCreateDefaultSubobject`**：派生一个 `ULightVehicleConfig : public UVehicleConfig`，在其构造函数里使用 `DoNotCreateDefaultSubobject(TEXT("EngineSpec"))` 阻止父类创建子对象，观察 CDO 的 `EngineSpec` 是否为 null。

4. **TObjectPtr access barrier 实验**（必做，这是 UE5 关键特性）：
   ```cpp
   // 在 Editor 构建下，读取 TObjectPtr 字段会触发 access tracking
   // ObjectPtr.h:778-927 中 GetAccessTrackedObjectPtr() 系列函数
   TObjectPtr<UEngineSpec> Ptr = CDO->EngineSpec;  // 读操作触发 ConditionallyMarkAsReachable
   
   // 观察：在 UE_OBJECT_PTR_GC_BARRIER == 1 的构建下（Editor），
   // 构造 FObjectPtr 时调用 ConditionallyMarkAsReachable(Object)
   // 在 UE_OBJECT_PTR_GC_BARRIER == 0 的构建下（Shipping 等），该调用被编译为 no-op
   ```
   在你的观察记录里写明：`TObjectPtr<T>` 在 Editor 构建中与裸 `T*` 的关键区别是什么？Shipping 中两者是否等价？

5. 尝试在构造函数**之外**（例如在一个普通成员函数里）调用 `CreateDefaultSubobject`，记录崩溃或断言信息，理解原因。

### 进阶任务

- 查看 `UObjectGlobals.h:1891-1953` 的 `NewObject<T>` 函数模板，理解它如何构建 `FStaticConstructObjectParameters` 并调用 `StaticConstructObject_Internal`；与 CDO 路径（`UClass::GetDefaultObject` → `StaticConstructObject_Internal` with `RF_ClassDefaultObject` flag）对比，写出两条路径的区别。
- 观察 Blueprint 派生类对 CDO 的覆写：在编辑器里创建一个继承自 `UVehicleConfig` 的 Blueprint，修改默认值，再通过 `GetDefaultObject<UVehicleConfig>()` 查询——Blueprint 子类的 CDO 会覆盖 C++ CDO 的默认值。
- 实验 `RF_ClassDefaultObject` 标志：调用 `CDO->HasAnyFlags(RF_ClassDefaultObject)` 验证 CDO 有此标志；普通 `NewObject` 创建的实例没有此标志。

### 验收点

- 你能解释 `CreateDefaultSubobject` 为什么只能在构造函数里调用：因为 `FObjectInitializer` 的生命周期严格绑定到对象构造期，其析构函数会最终确认所有 required subobject 都被创建；构造函数外调用时 `FObjectInitializer` 已不存在，UE 会触发 `check()` 失败。
- 你能说出 CDO 的创建时机：第一次调用 `UClass::GetDefaultObject(true)` 时（`Class.h:4373`），如果 CDO 尚未创建则调用 `StaticConstructObject_Internal` 并设 `RF_ClassDefaultObject` 标志。
- 你能对照源码说出 `TObjectPtr` 在 Editor 构建 (`UE_OBJECT_PTR_GC_BARRIER=1`, `ObjectPtr.h:23-26`) 与 Shipping 构建 (`UE_OBJECT_PTR_GC_BARRIER=0`) 的行为差异：Editor 构建每次构造 `FObjectPtr` 时调用 `ConditionallyMarkAsReachable`（`ObjectPtr.h:74-77`），Shipping 中该代码被条件编译掉。

### 观察点

**TObjectPtr access barrier（UE 特有机制 §10.4，必须显式理解）**：

`ObjectPtr.h` 中定义了 `UE_OBJECT_PTR_GC_BARRIER` 宏（默认值 1）。当构造一个 `FObjectPtr` / `TObjectPtr<T>` 时，Editor 构建下会调用：

```
ObjectPtr.h:71-77:
[[nodiscard]] explicit FORCEINLINE constexpr FObjectPtr(UObject* Object)
    : Handle(UE::CoreUObject::Private::MakeObjectHandle(Object))
{
#if UE_OBJECT_PTR_GC_BARRIER
    ConditionallyMarkAsReachable(Object);   // ← Editor 构建触发
#endif
}
```

`ConditionallyMarkAsReachable` 内部（`ObjectPtr.h:306-329`）：如果当前增量 GC 正在运行（通过 `UE::GC::MarkAsReachable`），会立即把该对象标记为可达，防止在 GC 运行期间被错误回收。

在 **Shipping** 构建中，`UE_OBJECT_PTR_GC_BARRIER` 可以被关闭，此时 `TObjectPtr<T>` 在内存布局和访问成本上与裸 `T*` 完全等价（零开销）。

这是 UE5 对 UE4 裸指针的核心升级点：`UPROPERTY TObjectPtr<T>` = UE4 `UPROPERTY T*` 的 drop-in 替换，Editor 下多了 GC barrier 和访问追踪，Shipping 下无开销。

**两套所有权对照**：

| 字段声明 | GC 可见？ | 所有权体系 | 何时安全 |
|---|---|---|---|
| `UPROPERTY() TObjectPtr<UObject> Ref` | 是 | GC（PropertyLink 链） | 推荐方式 |
| `UPROPERTY() UObject* Ref` | 是（UE4 风格） | GC | 兼容可用，UE5 建议换 TObjectPtr |
| `UObject* Ref`（无 UPROPERTY） | **否** | 无人管理 | GC 随时回收，形成悬空指针 |
| `TSharedPtr<FMyStruct> Ptr` | 否（FMyStruct 非 UObject） | C++ 引用计数 | 正确管理非 UObject 对象 |
| `TSharedPtr<UObject> Ptr` | **否** | C++ 引用计数（错误！） | **绝对不要这样做** |

- 源码参考：
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\UObjectGlobals.h:1363-1430`（`FObjectInitializer::CreateDefaultSubobject` 所有重载）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\UObjectGlobals.h:1889-1953`（`NewObject<T>` 三个重载，均内部调用 `StaticConstructObject_Internal`）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\Class.h:4373-4502`（`UClass::GetDefaultObject()`）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\ObjectPtr.h:23-77`（`UE_OBJECT_PTR_GC_BARRIER` 定义 + FObjectPtr 构造 + `ConditionallyMarkAsReachable`）

### 常见坑 / 提示

- **坑 1（最高频）**：在构造函数外调用 `CreateDefaultSubobject`——崩溃信息是 `ensure(CurrentInitializer)` 失败。解决：只在构造函数里调用；如果要在运行时动态添加子对象，应使用 `NewObject<T>(this, ...)` 并手动管理。
- **坑 2**：混用 `TSharedPtr<UObject>`。`TSharedPtr` 不与 GC 交互，`TSharedPtr` 引用计数归零会调用析构，但 GC 已经认为这个 UObject 不可达可以回收，产生双重析构。绝对禁止。
- **坑 3**：Blueprint 派生类对 C++ CDO 字段的覆写不会反映在 C++ 侧的 `GetDefaultObject<UCppClass>()`，而会反映在 Blueprint 子类的 `GetDefaultObject<UBPSubclass>()`，两者是不同的 CDO 实例。
- **提示**：用 `UObject::GetClass()->GetName()` 可以区分对象属于 C++ 类还是 Blueprint 类（Blueprint 类名包含 `_C` 后缀）。

### 复盘问题

- **Q1**：CDO 的构造时机和触发者是什么？（答：首次调用 `UClass::GetDefaultObject(true)` 时在 GameThread 上触发；Module 加载时也可能触发，取决于是否有代码提前访问 `StaticClass()`。）
- **Q2**：一个 `UVehicleConfig` 实例的生命周期拥有者是谁？（答：GC 管理；只要实例被 `UPROPERTY` 引用或 `AddToRoot()` 保护，就不会被回收；否则下次 `CollectGarbage` 时被清扫。）
- **Q3**：`CreateDefaultSubobject` 调用和 `NewObject` 调用各在哪条线程？（答：均在 GameThread。UObject 系统是单线程的，包括 CDO 构造。）
- **Q4**：`TObjectPtr<UEngineSpec> EngineSpec` 和 `UEngineSpec* EngineSpec`（都带 UPROPERTY）对 GC 可见性是否相同？（答：相同——两者都让 GC 通过 PropertyLink 发现引用。差别在 Editor 下 `TObjectPtr` 触发 GC barrier，`UEngineSpec*` 不触发。）

### 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1363` (`CreateDefaultSubobject`)
- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1891` (`NewObject<T>`)
- `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h:23` (`UE_OBJECT_PTR_GC_BARRIER`)
- `Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:4373` (`GetDefaultObject`)

---

## D3 — 反射遍历 FProperty

### 目标

用代码遍历 `UClass::PropertyLink` 链，按 `FProperty` 子类型分发，dump 每个 `UPROPERTY` 的名字、类型、内存偏移和标志。理解 UE 反射系统是如何把 `UPROPERTY` 宏变成运行时可查询的元数据的。

**阶梯定位**：inspect——读懂实现层，理解 PropertyLink 的数据结构。

### 前置理解

- 你已经完成 D1/D2，理解 UHT 在 `.gen.cpp` 里生成的 `FObjectPropertyDesc` / `FIntPropertyDesc` 等描述符，以及它们如何在 Module 加载时注册到 `UClass`。
- 你知道 `UClass` 继承自 `UStruct`，`PropertyLink` 是 `UStruct` 的字段（`Class.h:530`）。
- 你了解 `FProperty` 是所有属性描述符的基类（位于 `UnrealType.h`），`FBoolProperty`、`FIntProperty`、`FObjectProperty`、`FArrayProperty`、`FStructProperty` 是其子类。

### 必做任务

1. 创建 Module `D3_ReflectionWalk`，定义一个包含多种字段类型的 `UCLASS`：
   ```cpp
   UCLASS()
   class D3REFLECTIONWALK_API UReflectTarget : public UObject
   {
       GENERATED_BODY()
   public:
       UPROPERTY(EditAnywhere) bool          bActive = true;
       UPROPERTY(EditAnywhere) int32         Count = 0;
       UPROPERTY(EditAnywhere) float         Weight = 1.f;
       UPROPERTY(EditAnywhere) FString       Label = TEXT("hello");
       UPROPERTY(EditAnywhere) TObjectPtr<UObject> ChildRef;
       UPROPERTY(EditAnywhere) TArray<int32> Numbers;
       // 故意不加 UPROPERTY 的字段：
       int32                                 HiddenField = 42;
   };
   ```

2. 写一个工具函数 `DumpClassProperties(UClass* Class)`，遍历 `PropertyLink` 链并按类型分发：
   ```cpp
   void DumpClassProperties(UClass* Class)
   {
       // PropertyLink 链从最派生类到基类，依次遍历
       // Class.h:530: FProperty* PropertyLink;
       for (FProperty* Prop = Class->PropertyLink; Prop; Prop = Prop->PropertyLinkNext)
       {
           FString TypeName;

           if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
           {
               TypeName = TEXT("bool");
           }
           else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
           {
               TypeName = TEXT("int32");
           }
           else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
           {
               TypeName = TEXT("float");
           }
           else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
           {
               TypeName = TEXT("FString");
           }
           else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
           {
               TypeName = FString::Printf(TEXT("TObjectPtr<%s>"), 
                   *ObjProp->PropertyClass->GetName());
           }
           else if (FArrayProperty* ArrProp = CastField<FArrayProperty>(Prop))
           {
               TypeName = FString::Printf(TEXT("TArray<...>"));
           }
           else
           {
               TypeName = Prop->GetClass()->GetName();
           }

           UE_LOG(LogTemp, Warning, 
               TEXT("  [%s] Name=%s Offset=%d Flags=0x%llX"),
               *TypeName,
               *Prop->GetName(),
               Prop->GetOffset_ForInternal(),
               (uint64)Prop->PropertyFlags
           );
       }
   }
   ```

3. 在 `StartupModule` 里调用 `DumpClassProperties(UReflectTarget::StaticClass())`，观察：`HiddenField` 是否出现在输出中？为什么？

4. **用 `TFieldIterator` 改写**（UE 推荐方式）：
   ```cpp
   // TFieldIterator 遍历所有 FProperty（含继承链）
   for (TFieldIterator<FProperty> PropIt(UReflectTarget::StaticClass()); PropIt; ++PropIt)
   {
       FProperty* Prop = *PropIt;
       UE_LOG(LogTemp, Warning, TEXT("TFieldIterator: %s"), *Prop->GetName());
   }
   ```
   对比：手动遍历 `PropertyLink` vs `TFieldIterator<FProperty>` —— 两者的覆盖范围（含/不含基类属性）有何差异？

5. **通过 FObjectProperty 写入值**：
   ```cpp
   UReflectTarget* Target = NewObject<UReflectTarget>(GetTransientPackage());
   FObjectProperty* ChildRefProp = CastField<FObjectProperty>(
       UReflectTarget::StaticClass()->FindPropertyByName(TEXT("ChildRef")));
   
   UObject* NewChild = NewObject<UObject>(Target);
   ChildRefProp->SetObjectPropertyValue_InContainer(Target, NewChild);
   
   // 验证
   UObject* Retrieved = ChildRefProp->GetObjectPropertyValue_InContainer(Target);
   check(Retrieved == NewChild);
   ```

6. **反射调 `UFUNCTION`**（进阶预热）：给 `UReflectTarget` 加一个 `UFUNCTION()`：
   ```cpp
   UFUNCTION() void OnPropertyChanged();
   ```
   用 `UReflectTarget::StaticClass()->FindFunctionByName(TEXT("OnPropertyChanged"))` 找到 `UFunction`，再用 `Target->ProcessEvent(Func, nullptr)` 调用它，观察日志。

### 进阶任务

- 遍历 `UObject` 基类的 `PropertyLink`，观察 `UObject` 自身有多少个 `UPROPERTY`（通常为 0 或极少，大多数基类字段不暴露给反射）。
- 实现一个简单的"JSON 序列化器"：遍历 `PropertyLink`，把所有基础类型字段（bool/int32/float/FString）序列化为 JSON 字符串，验证反射驱动序列化的可行性。
- 研究 `FProperty::EPropertyFlags`（位于 `ObjectMacros.h`），找出 `CPF_Net`（网络 replication 相关）、`CPF_Edit`（编辑器可见）、`CPF_BlueprintVisible` 的值，理解 `UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)` 是如何映射到这些标志位的。

### 验收点

- 你能徒手画出 `UClass::PropertyLink` 链的内存结构（单向链表，每个节点是 `FProperty`，通过 `PropertyLinkNext` 连接）。
- 你能说出 `HiddenField`（无 UPROPERTY）不出现在 PropertyLink 中的根本原因：UHT 根本不为它生成 `FIntProperty` 描述符，因此 PropertyLink 链里没有对应节点。
- 你能区分 `CastField<T>` 和 `Cast<T>`：前者用于 `FField` 及其子类（`FProperty` 族），后者用于 `UObject` 及其子类；两者不可互换。
- 你能通过 `FObjectProperty::SetObjectPropertyValue_InContainer` 写入一个字段，并用 `GetObjectPropertyValue_InContainer` 验证读取正确。

### 观察点

**反射可见性图**（D 模块触发，必画）：

```
UReflectTarget（UCLASS）
├── UPROPERTY 字段（UHT 可见 / GC 可见 / 编辑器可见）
│   ├── bActive      → FBoolProperty    （offset=X, flags=CPF_Edit|CPF_BlueprintVisible）
│   ├── Count        → FIntProperty
│   ├── Weight       → FFloatProperty
│   ├── Label        → FStrProperty
│   ├── ChildRef     → FObjectProperty  （PropertyClass=UObject, GC 通过此 prop 遍历引用）
│   └── Numbers      → FArrayProperty   （Inner=FIntProperty）
└── 普通 C++ 字段（UHT 不可见 / GC 不可见）
    └── HiddenField  → 仅 C++ 编译器可见，反射/GC 均看不到
```

**PropertyLink vs RefLink**（来自 `Class.h:529-536`）：
- `PropertyLink`：所有 `UPROPERTY` 字段的链表（最派生到基类）
- `RefLink`：仅包含**持有 UObject 引用**的属性（`FObjectProperty` / `FWeakObjectProperty` / `FSoftObjectProperty` 等）
- GC 的可达性分析主要遍历 `RefLink`，不是全量的 `PropertyLink`，这是性能优化——只需要跟踪 UObject 指针，不需要遍历 `int32` / `float` 等基础类型字段。

- 源码参考：
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\UnrealType.h:211`（`FProperty::PropertyLinkNext`）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\UnrealType.h:2542`（`FBoolProperty`）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\UnrealType.h:3086`（`FObjectProperty`）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\UnrealType.h:3701`（`FArrayProperty`）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\Class.h:530`（`UStruct::PropertyLink` 字段）

### 常见坑 / 提示

- **坑 1**：用 `Cast<FProperty>` 代替 `CastField<FProperty>` 编译失败。`FProperty` 从 UE4.25 起不再继承自 `UObject`，使用的是 `FField` 体系，`Cast<>` 不适用。
- **坑 2**：遍历时修改 PropertyLink 链（例如在循环里调 `AddProperty`）导致迭代器失效。遍历期间不要修改链表。
- **坑 3**：`GetOffset_ForInternal()` 返回的是相对于对象起始地址的字节偏移，直接 `(uint8*)Object + Offset` 就能取到字段地址——这正是 GC 遍历和序列化系统的底层机制。
- **提示**：`TFieldIterator<FProperty>(Class, EFieldIteratorFlags::IncludeSuper)` 会同时遍历继承自基类的属性；`EFieldIteratorFlags::ExcludeSuper` 只遍历本类定义的属性。

### 复盘问题

- **Q1**：`DumpClassProperties` 的遍历发生在哪条线程的什么时刻？（答：GameThread，`StartupModule` 调用时，即 Module 加载阶段。此时 `UClass` 的 PropertyLink 已经被 UHT 生成的 `.gen.cpp` 注册完毕。）
- **Q2**：`UReflectTarget` 实例的生命周期拥有者？（答：GC 管理。`NewObject<UReflectTarget>(GetTransientPackage())` 后若无 UPROPERTY 引用也无 AddToRoot，下次 GC 即回收。）
- **Q3**：Named Thread？（答：纯 GameThread。反射查询是只读的，不涉及 RenderThread 或 RHIThread。）
- **Q4**：`ChildRef` 字段如何让 GC 知道它持有 UObject 引用？（答：UHT 在 `.gen.cpp` 里为它生成 `FObjectProperty` 节点，注册到 `UClass::RefLink`；GC 遍历 `RefLink` 时读取 `ChildRef` 字段的指针值，调用 `UE::GC::MarkAsReachable` 标记目标对象。）
- 为什么 GC 用 `RefLink` 而不是 `PropertyLink` 做可达性分析？（答：性能——只有包含 UObject 引用的属性才需要追踪；`int32`/`float`/`FString` 字段无法持有 UObject 引用，跳过它们可以把 GC 遍历耗时降低一个数量级。）

### 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:211`（`PropertyLinkNext`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:2542,3086,3701`（`FBoolProperty`,`FObjectProperty`,`FArrayProperty`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:530`（`UStruct::PropertyLink`）

---

## D4 — 手写 mini mark-sweep GC（implement-own，核心输出）

### 目标

这是本课程**第一次触底**。自己手写一个最小的 mark-sweep GC（`MiniGC`），把"为什么 UPROPERTY 缺失导致悬空"的底层机制彻底理解——不是凭记忆，而是因为你亲手写过它。

做完本题，你应该能画出以下五层关系图，并用代码回答每一层：

```
┌─────────────────────────────────────────────────────────────┐
│  GMiniObjectArray（全局对象注册表）                          │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌──────────┐ │
│  │ MiniObj A │  │ MiniObj B │  │ MiniObj C │  │MiniObj D │ │
│  │ RootSet=T │  │ RootSet=F │  │ RootSet=F │  │RootSet=F │ │
│  │ Refs→[B,C]│  │ Refs→[D]  │  │ Refs→[]   │  │ Refs→[]  │ │
│  └───────────┘  └───────────┘  └───────────┘  └──────────┘ │
│                                                              │
│  Mark Phase（从 RootSet 出发 BFS/DFS）：                     │
│    A 可达（root）→ B 可达（A引用）→ C 可达（A引用）→ D 可达 │
│                                                              │
│  假设 C 从 A 的 Refs 中移除（类比"丢失 UPROPERTY"）：        │
│    A 可达 → B 可达 → D 可达；C 不可达 → Sweep 阶段删除 C    │
└─────────────────────────────────────────────────────────────┘
```

### 前置理解

- 你已经完成 D1-D3，理解 PropertyLink 是反射遍历的基础。
- 你从 `01-心智模型` §4 理解了 GC 的 mark-sweep 原理。
- 你理解两套所有权：UPROPERTY/TObjectPtr（GC 可见）vs TSharedPtr/裸指针（GC 不可见）。

### 必做任务

1. 实现 `MiniObject` 基类：

   ```cpp
   // MiniObject.h — 独立的纯 C++ 文件，不依赖 UE UObject 系统
   #pragma once
   #include <vector>
   #include <string>
   #include <functional>

   // 模拟 GUObjectArray 的全局注册表
   extern std::vector<class MiniObject*> GMiniObjectArray;

   class MiniObject
   {
   public:
       std::string Name;

       // 模拟 RF_RootSet 标志
       bool bIsRootSet = false;

       // GC 标记位（mark phase 期间设置）
       bool bReachable = false;

       // 模拟 UPROPERTY 引用链：手动注册需要被 GC 追踪的引用
       // 这等价于 UStruct::RefLink —— 只追踪能持有 MiniObject* 的字段
       std::vector<MiniObject**> TrackedRefs;

       explicit MiniObject(std::string InName) : Name(std::move(InName))
       {
           // 相当于 NewObject 后自动注册到 GUObjectArray
           GMiniObjectArray.push_back(this);
       }

       virtual ~MiniObject() = default;

       // 模拟 AddToRoot：加入根集后 GC 不会回收
       void AddToRoot() { bIsRootSet = true; }
       void RemoveFromRoot() { bIsRootSet = false; }

       // 手动注册一个需要 GC 追踪的引用字段
       // 类比：给一个成员指针加 UPROPERTY 标注
       void TrackRef(MiniObject** RefField) { TrackedRefs.push_back(RefField); }
   };
   ```

2. 实现 `MiniGC`，包含 Mark 和 Sweep 两阶段：

   ```cpp
   // MiniGC.h
   #pragma once
   #include "MiniObject.h"

   class MiniGC
   {
   public:
       // === Mark Phase ===
       // 从 RootSet 出发，递归标记所有可达对象
       static void Mark()
       {
           // 清除上一次 GC 的标记
           for (MiniObject* Obj : GMiniObjectArray)
           {
               if (Obj) Obj->bReachable = false;
           }

           // 收集根集（bIsRootSet == true 的对象）
           // 类比：UE 的 GRoots —— GarbageCollection.cpp:638
           std::vector<MiniObject*> WorkList;
           for (MiniObject* Obj : GMiniObjectArray)
           {
               if (Obj && Obj->bIsRootSet)
               {
                   Obj->bReachable = true;
                   WorkList.push_back(Obj);
               }
           }

           // BFS 可达性传递
           // 类比：PerformReachabilityAnalysis（GarbageCollection.cpp:4528）
           while (!WorkList.empty())
           {
               MiniObject* Current = WorkList.back();
               WorkList.pop_back();

               // 遍历该对象注册的所有 GC 可见引用
               // 类比：遍历 UStruct::RefLink
               for (MiniObject** RefField : Current->TrackedRefs)
               {
                   MiniObject* Referenced = *RefField;
                   if (Referenced && !Referenced->bReachable)
                   {
                       Referenced->bReachable = true;
                       WorkList.push_back(Referenced);
                   }
               }
           }
       }

       // === Sweep Phase ===
       // 删除所有未标记的对象
       static void Sweep()
       {
           for (MiniObject*& Obj : GMiniObjectArray)
           {
               if (Obj && !Obj->bReachable)
               {
                   printf("[MiniGC::Sweep] Collecting: %s\n", Obj->Name.c_str());
                   delete Obj;
                   Obj = nullptr;  // 留 null 槽，不立即压缩数组（类比 UE 的延迟清理）
               }
           }

           // 压缩数组（UE 实际上不做这步，用 FUObjectItem 的 IsValid 标志代替）
           GMiniObjectArray.erase(
               std::remove(GMiniObjectArray.begin(), GMiniObjectArray.end(), nullptr),
               GMiniObjectArray.end()
           );
       }

       // === Full GC Cycle ===
       static void Collect()
       {
           printf("[MiniGC] === GC Cycle Start ===\n");
           Mark();
           Sweep();
           printf("[MiniGC] === GC Cycle End (%zu objects alive) ===\n",
               GMiniObjectArray.size());
       }
   };
   ```

3. 写一个验证主程序（在 `StartupModule` 或独立 test 函数里）：

   ```cpp
   void RunMiniGCDemo()
   {
       // 创建对象图：A→B→D, A→C
       MiniObject* A = new MiniObject("A");
       MiniObject* B = new MiniObject("B");
       MiniObject* C = new MiniObject("C");
       MiniObject* D = new MiniObject("D");

       // 模拟 A.UPROPERTY TObjectPtr<B> RefB; A.UPROPERTY TObjectPtr<C> RefC;
       A->TrackRef(&B);  // A 持有对 B 的 GC 可见引用
       A->TrackRef(&C);  // A 持有对 C 的 GC 可见引用
       B->TrackRef(&D);  // B 持有对 D 的 GC 可见引用

       // A 是根（AddToRoot）
       A->AddToRoot();

       // 第一次 GC：A/B/C/D 全部可达，无对象被回收
       printf("\n--- GC Round 1 (all reachable) ---\n");
       MiniGC::Collect();

       // 模拟"丢失 UPROPERTY"：A 不再追踪 C（类比去掉 UPROPERTY 标注）
       A->TrackedRefs.erase(
           std::remove(A->TrackedRefs.begin(), A->TrackedRefs.end(), &C),
           A->TrackedRefs.end()
       );
       // C 仍然活在内存里，但 GC 看不到它了——悬空的根因

       // 第二次 GC：C 不可达，被回收；A/B/D 可达
       printf("\n--- GC Round 2 (C unreachable after losing UPROPERTY) ---\n");
       MiniGC::Collect();
       // 此后访问 C 是 undefined behavior（dangling pointer）
       // 这就是"裸 UObject* 成员不加 UPROPERTY"的后果

       // 清理
       A->RemoveFromRoot();
       MiniGC::Collect();
   }
   ```

4. **实现增量 GC 模拟**（必做进阶）：把 `Mark()` 改成每次只处理 `MaxObjectsPerFrame` 个对象，跨帧完成标记，模拟 UE 的增量可达性分析：

   ```cpp
   // 增量标记状态
   static std::vector<MiniObject*> IncrementalWorkList;
   static bool bIncrementalMarkInProgress = false;

   static bool IncrementalMarkStep(int MaxObjectsPerStep)
   {
       if (!bIncrementalMarkInProgress)
       {
           // 初始化：清标记，加根集
           for (MiniObject* Obj : GMiniObjectArray)
               if (Obj) Obj->bReachable = false;
           IncrementalWorkList.clear();
           for (MiniObject* Obj : GMiniObjectArray)
               if (Obj && Obj->bIsRootSet) { Obj->bReachable = true; IncrementalWorkList.push_back(Obj); }
           bIncrementalMarkInProgress = true;
       }

       int Processed = 0;
       while (!IncrementalWorkList.empty() && Processed < MaxObjectsPerStep)
       {
           MiniObject* Current = IncrementalWorkList.back();
           IncrementalWorkList.pop_back();
           for (MiniObject** RefField : Current->TrackedRefs)
           {
               MiniObject* Referenced = *RefField;
               if (Referenced && !Referenced->bReachable)
               {
                   Referenced->bReachable = true;
                   IncrementalWorkList.push_back(Referenced);
               }
           }
           ++Processed;
       }

       if (IncrementalWorkList.empty())
       {
           bIncrementalMarkInProgress = false;
           return true; // Mark phase complete
       }
       return false; // Still in progress
   }
   ```

5. 对照 UE 源码 `GarbageCollection.cpp:4528-4577`（`PerformReachabilityAnalysis`）——理解它的 `while (true)` 主循环、`IsSuspended()` 检查、以及 `EGCOptions::IncrementalReachability` 标志如何控制"分帧完成标记"的行为。

### 进阶任务

- 实现 `FGCObject` 等价物：一个非 `MiniObject` 的类，但它持有 `MiniObject*` 引用，并通过注册回调告诉 `MiniGC` 自己持有的引用——模拟 UE 的 `FGCObject::AddReferencedObjects` 接口。
- 实现对象集群（Cluster）模拟：把一组相互引用的 `MiniObject` 打包成一个"集群"，集群整体作为 GC 单元，只要集群入口可达则整组对象都存活——这对应 UE 的 `CreateCluster` 机制（GC 优化之一）。
- 研究 `GarbageCollection.cpp:634-638` 中的 `GGatherUnreachableObjectsState`（`TThreadedGather`）——UE 的 Sweep phase 是多线程并行的，未标记对象的收集使用 Worker Thread Pool 并行化。

### 验收点

- 你的 `RunMiniGCDemo()` 正确输出"第一次 GC 无回收，第二次 GC 回收 C，第三次 GC 回收 A/B/D"。
- 你能用一句话解释：为什么"裸 `UObject*` 成员不加 `UPROPERTY`"会导致 GC 回收你以为还活着的对象——因为 `MiniGC::Mark()` 只能通过 `TrackedRefs`（对应 `PropertyLink`/`RefLink`）发现引用，不加 `TrackRef()` 等价于不加 `UPROPERTY`，GC 看不到这条引用。
- 你能说明 stop-the-world GC（`Collect()` 一次性完成）与增量 GC（`IncrementalMarkStep(N)` 分帧）的差别：前者会暂停游戏线程（帧率尖峰），后者把标记工作分散到多帧，每帧只处理 N 个对象，代价是标记期间新产生的引用需要额外处理（write barrier / GC barrier，即 `TObjectPtr` 的 `ConditionallyMarkAsReachable`）。
- 你能把"UObject 元素 / PropertyLink / RootSet / ReachableSet / UnreachableSet"画成一张关系图（见模块目标的 ASCII 图）并标注五元素。

### 观察点

**增量 GC 与 TObjectPtr GC Barrier 的关系**（这是理解 UE5 TObjectPtr 的核心）：

在增量 GC 期间（`EGCOptions::IncrementalReachability`），GC 的 Mark phase 分多帧运行。如果 GameThread 在 GC 运行期间**创建一个新的 TObjectPtr 指向某个对象**，而该对象尚未被 GC 遍历到，就可能被错误地清扫（经典的"漏标"问题）。

`TObjectPtr` 的 `ConditionallyMarkAsReachable`（`ObjectPtr.h:306-329`）正是解决这个问题的 write barrier：每次赋值/构造 `TObjectPtr` 时，如果增量 GC 正在运行，**立即把目标对象标记为可达**，防止漏标。

在 `MiniGC` 的增量实现中，等价逻辑是：如果在 `IncrementalMarkStep` 调用之间，你修改了某个 `MiniObject*` 引用，你需要手动调用 `Referenced->bReachable = true`（相当于 `UE::GC::MarkAsReachable`）。

**对照 UE 源码**：
- `GarbageCollection.cpp:316-334`：`GAllowIncrementalGather` / `GIncrementalGatherTimeLimit` 控制变量
- `GarbageCollection.cpp:4528-4577`：`PerformReachabilityAnalysis` 主循环，`IsSuspended()` 检查增量状态
- `GarbageCollection.cpp:4552`：`EnumHasAnyFlags(Options, EGCOptions::IncrementalReachability)` 判断是否分帧
- `UObjectBaseUtility.h:206-208`：`AddToRoot()` 实现——调用 `GUObjectArray.IndexToObject(InternalIndex)->SetRootSet()`
- `ObjectPtr.h:306-329`：`FObjectPtr::ConditionallyMarkAsReachable` 增量 GC barrier

**RF_RootSet 在 UE 中的位置**：

`AddToRoot()` 设置的不是 `EObjectFlags`（`RF_*` 前缀）里的标志，而是 `EInternalObjectFlags::RootSet`（内部标志，在 `GUObjectArray` 的 `FUObjectItem` 上），`UObjectBaseUtility.h:206-208`：

```cpp
void AddToRoot()
{
    GUObjectArray.IndexToObject(InternalIndex)->SetRootSet();
}
```

这意味着 `HasAnyFlags(RF_RootSet)` 在现代 UE 版本中不是正确的检测方式，应该用 `IsRooted()` 或检查 `EInternalObjectFlags::RootSet`。`RF_RootSet` 是早期版本遗留命名，在 UE5 中已经迁移到内部标志。

- 源码参考：
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\UObjectBaseUtility.h:206-215`（`AddToRoot()`/`RemoveFromRoot()`）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\GarbageCollection.h:179-212`（`GIsGarbageCollecting`、`IsGarbageCollecting()`）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Private\UObject\GarbageCollection.cpp:316-340`（增量 GC 控制变量）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Private\UObject\GarbageCollection.cpp:634-638`（GRoots、GGatherUnreachableObjectsState）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Private\UObject\GarbageCollection.cpp:4528-4597`（`PerformReachabilityAnalysis` 主循环）
  - `G:\Unreal Engine\Source\UnrealEngine\Engine\Source\Runtime\CoreUObject\Public\UObject\ObjectPtr.h:306-329`（`ConditionallyMarkAsReachable`）

### 常见坑 / 提示

- **坑 1**：`MiniGC` 的 `Sweep` 阶段在 Mark 阶段**之前**回收对象。顺序必须是先 Mark 再 Sweep，否则活跃对象会被删除。
- **坑 2**：增量 GC 的漏标问题。如果在 `IncrementalMarkStep` 调用之间修改了对象的引用关系（添加了新引用），必须手动将新引用的目标对象加入工作列表或直接标记为可达。这就是为什么 `TObjectPtr` 需要 write barrier。
- **坑 3**：`bReachable` 标记是 per-GC-cycle 的，每次 GC 开始必须清零。如果忘记清零，对象永远是"可达的"，GC 永远无法回收任何对象。
- **坑 4**：在 Sweep 阶段删除对象时不要直接用迭代器擦除（会导致循环失效），先设 null 再统一清理（UE 的实际做法是标记 `EInternalObjectFlags::Garbage` 而不是立即删除）。
- **提示**：UE 的 GC 不会立即调用析构函数，而是先标记 `RF_Garbage`，在 Sweep 阶段调用 `ConditionalBeginDestroy()` / `ConditionalFinishDestroy()`，最终在下一帧的 Purge 阶段才真正释放内存。这个"延迟析构"设计让 UObject 可以在 `BeginDestroy` 中异步清理资源。

### 复盘问题

- **Q1（执行真正开始时刻）**：UE 的 `CollectGarbage` 在哪里被触发？（答：在 GameThread 的帧间空隙，`UEngine::ConditionalCollectGarbage`；也可显式调用 `CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS)`。GC 绝不在帧执行期间中途介入普通代码。）
- **Q2（生命周期拥有者）**：`MiniGC` 场景里，A/B/D 的生命周期拥有者是谁？（答：`MiniGC` 本身——等价于 UE 的 GC 系统；通过 `bIsRootSet` 和 `TrackedRefs` 链维持可达性，等价于 UE 的 `GRoots` + `PropertyLink/RefLink`。）
- **Q3（Named Thread）**：UE 的 Mark phase 在哪条线程运行？（答：主要在 GameThread；`GAllowParallelGC=1` 时会并行到 Worker Thread Pool 加速标记（`EGCOptions::Parallel`），但 GC 启动和 Sweep 仍在 GameThread。）
- **Q4（GC 可见性）**：如果你把 `MiniObject* C` 存在一个 `std::vector<MiniObject*>` 而没有通过 `TrackRef` 注册，GC 能看见它吗？（答：不能——等价于把 `UObject*` 存在一个非 UPROPERTY 的 `TArray<UObject*>` 里，GC 不遍历它，C 会被回收，访问 C 变成悬空指针。对应 UE 的修复方案：把容器声明为 `UPROPERTY() TArray<TObjectPtr<UObject>> Items`。）
- **设计题**：为什么 UE 选择 mark-sweep 而不是引用计数（`TSharedPtr`）来管理 UObject？（答：引用计数无法处理循环引用（Actor → Component → Actor 是常见的循环）；mark-sweep 可以正确处理任意图结构；此外 mark-sweep 与反射元数据（PropertyLink）天然结合，不需要在每个指针赋值处手动维护计数。代价是 stop-the-world 或增量 GC 的复杂性。）

### 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBaseUtility.h:206`（`AddToRoot`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/GarbageCollection.h:179`（`GIsGarbageCollecting`）
- `Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:4528`（`PerformReachabilityAnalysis`）
- `Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:634`（`GRoots`、`GGatherUnreachableObjectsState`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h:306`（`ConditionallyMarkAsReachable`，增量 GC barrier）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:930`（`CollectGarbage` 声明）

---

## 做完模块 D 后你现在应该能说清楚什么

1. **UHT 流程**：UHT 在 C++ 编译器之前跑，读取含 `UCLASS`/`UPROPERTY`/`UFUNCTION` 的头文件，生成 `.generated.h` 和 `.gen.cpp`。`GENERATED_BODY()` 展开为三块声明：`CALLBACK_WRAPPERS`（蓝图接入点）、`INCLASS_NO_PURE_DECLS`（StaticClass/StaticRegisterNatives）、`ENHANCED_CONSTRUCTORS`（删除的拷贝构造/虚析构/CDO 初始化）。

2. **CDO 时机**：CDO 在第一次访问 `UClass::GetDefaultObject(true)` 时创建，带 `RF_ClassDefaultObject` 标志；`CreateDefaultSubobject` 严格只能在构造期（`FObjectInitializer` 生命周期内）调用。

3. **PropertyLink 结构**：`UStruct::PropertyLink` 是 `FProperty*` 链表，从最派生类到基类，每个节点对应一个 `UPROPERTY` 字段。`RefLink` 是其子集，只包含持有 UObject 引用的属性，是 GC 可达性分析的遍历目标。

4. **TObjectPtr 差异**：Editor 构建 `UE_OBJECT_PTR_GC_BARRIER=1`，每次构造/赋值 `TObjectPtr` 调用 `ConditionallyMarkAsReachable`，为增量 GC 提供 write barrier；Shipping 构建该代码被条件编译掉，`TObjectPtr<T>` 内存布局等价裸 `T*`，零开销。

5. **GC 机制**：mark-sweep 两阶段。Mark phase：从 `GRoots`（`AddToRoot()`/`RF_ClassDefaultObject` 等标志的对象集合）出发，沿 `RefLink` BFS 标记可达对象；Sweep phase：删除未标记对象（先调 `ConditionalBeginDestroy`，再延迟 Purge）。增量 GC 把 Mark phase 分帧执行，write barrier（`TObjectPtr`）防止分帧期间新引用被漏标。

6. **两套所有权绝对分界**：
   - GC 路径：`UPROPERTY() TObjectPtr<T>` / `UPROPERTY() T*` / `AddToRoot()` / Outer 链 → GC 可见，GC 管理生命周期
   - C++ 路径：`TSharedPtr<FMyStruct>` / 裸 `T*`（无 UPROPERTY） / `TUniquePtr<T>` → GC 完全不可见，C++ RAII 管理生命周期
   - **混淆规则**：永远不要用 `TSharedPtr` 持有 `UObject*`；永远不要用无 `UPROPERTY` 的裸指针成员持有 `UObject*`（GC 不可见 → 悬空指针）。

---

## 本模块覆盖的 UE 源码清单

| 序号 | 文件路径 | 关键内容 |
|---|---|---|
| 1 | `Engine/Source/Runtime/CoreUObject/Public/UObject/Object.h:93-120` | `UObject` 基类定义，`UCLASS(Abstract, MinimalAPI)`，`GENERATED_BODY()`，四个构造函数重载 |
| 2 | `Engine/Intermediate/Build/Win64/UnrealEditor/Inc/CoreUObject/UHT/Object.generated.h:21-64` | `.generated.h` 展开结构样本：`INCLASS_NO_PURE_DECLS`/`ENHANCED_CONSTRUCTORS`/`GENERATED_BODY` 三块拼接，`Z_Construct_UClass_UObject_NoRegister` 声明 |
| 3 | `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:604` | `StaticConstructObject_Internal` 声明（所有 UObject 构造的底层入口） |
| 4 | `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1363-1430` | `FObjectInitializer::CreateDefaultSubobject` 所有重载、`DoNotCreateDefaultSubobject` |
| 5 | `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1889-1953` | `NewObject<T>` 三个模板重载，均通过 `FStaticConstructObjectParameters` 调用 `StaticConstructObject_Internal` |
| 6 | `Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:529-536` | `UStruct::PropertyLink`/`RefLink`/`DestructorLink`/`PostConstructLink` 四条链表字段 |
| 7 | `Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:4373-4502` | `UClass::GetDefaultObject()`，CDO 懒初始化入口 |
| 8 | `Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:211` | `FProperty::PropertyLinkNext`（链表 next 指针） |
| 9 | `Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:2542,3086,3701,6305` | `FBoolProperty`/`FObjectProperty`/`FArrayProperty`/`FStructProperty` 类定义 |
| 10 | `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h:23-77` | `UE_OBJECT_PTR_GC_BARRIER` 宏，`FObjectPtr` 构造函数中的 `ConditionallyMarkAsReachable` |
| 11 | `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectPtr.h:306-329` | `FObjectPtr::ConditionallyMarkAsReachable` 实现，增量 GC write barrier |
| 12 | `Engine/Source/Runtime/CoreUObject/Public/UObject/WeakObjectPtr.h:48-100` | `FWeakObjectPtr` 基础结构，`UE_WEAKOBJECTPTR_ZEROINIT_FIX`，零初始化语义 |
| 13 | `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBaseUtility.h:206-215` | `AddToRoot()`/`RemoveFromRoot()` 实现，操作 `GUObjectArray` 的 `SetRootSet`/`ClearRootSet` |
| 14 | `Engine/Source/Runtime/CoreUObject/Public/UObject/GarbageCollection.h:179-212` | `GIsGarbageCollecting`，`IsGarbageCollecting()`，`FGCScopeGuard` |
| 15 | `Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:316-340` | 增量 GC 控制变量：`GAllowIncrementalGather`/`GIncrementalGatherTimeLimit` |
| 16 | `Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:634-701` | `GRoots`（`TSet<int32>`），`GGatherUnreachableObjectsState`，根集的"dirty root"懒更新机制 |
| 17 | `Engine/Source/Runtime/CoreUObject/Private/UObject/GarbageCollection.cpp:4528-4597` | `PerformReachabilityAnalysis` 主循环：`while(true)` 遍历可达对象，`IsSuspended()` 增量中断检查，`EGCOptions::IncrementalReachability` 分帧控制 |

---

## 附录：两套所有权边界速查卡

```
┌──────────────────────────────────────────────────────────────────┐
│                    GC 可见（UObject 所有权）                       │
│                                                                  │
│  UPROPERTY() TObjectPtr<UMyObj> Ref;     ← 推荐（UE5）           │
│  UPROPERTY() UMyObj* Ref;                ← 可用（UE4 兼容）       │
│  MyObj->AddToRoot();                     ← 强制保活（根集）        │
│  // Outer 链：NewObject(this, ...) 后     ← 通过 Outer 间接可达    │
│                                                                  │
├──────────────────────────────────────────────────────────────────┤
│                    GC 不可见（C++ 所有权）                         │
│                                                                  │
│  UMyObj* Ref;                            ← 无 UPROPERTY → 悬空！ │
│  TSharedPtr<UMyObj> Ref;                 ← 绝对禁止              │
│  TSharedPtr<FMyStruct> Ref;              ← 正确（非 UObject）     │
│  TUniquePtr<FMyStruct> Ref;              ← 正确（非 UObject）     │
│  TArray<UMyObj*> Arr;                    ← 无 UPROPERTY → 悬空！ │
│  UPROPERTY() TArray<TObjectPtr<UMyObj>>  ← 正确（UE5）           │
└──────────────────────────────────────────────────────────────────┘

TObjectPtr<T> Editor vs Shipping：
  Editor (UE_OBJECT_PTR_GC_BARRIER=1):
    构造/赋值 → ConditionallyMarkAsReachable() → 防止增量 GC 漏标
    (ObjectPtr.h:74, ObjectPtr.h:306-329)
  Shipping (UE_OBJECT_PTR_GC_BARRIER=0):
    构造/赋值 → 无额外操作 → 零开销，内存布局等价裸 T*
```
