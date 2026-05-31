> 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-1

## 目标

写出第一个 `UCLASS`，观察 UHT 代码生成机制，逐段读懂 `.generated.h`，把"UHT 在 C++ 编译器之前跑"从抽象概念变成眼见为实的体验。理解 CDO（Class Default Object）的构造时机，以及 `HasAnyFlags(RF_ClassDefaultObject)` 区分 CDO 与实例的方法。

## 前置理解

- 已读 `01-心智模型.md` §2（Reflection before types），知道 `.generated.h` 的必要性。
- 知道 `#include "MyClass.generated.h"` 必须是头文件的最后一个 `#include`（UHT 硬性要求）。
- 已完成 ExA1_HelloModule，理解 UE Module 加载机制。

## 必做任务

1. 取消 `UExD1SampleObject` 头文件中 `Counter` 的 `UPROPERTY` 注释，触发 UHT 代码生成，在 `Intermediate/Build/.../Inc/ExD1_HelloUObject/UHT/` 找到 `.generated.h` 并打开阅读。
2. 在 `.generated.h` 中逐段分析：`GENERATED_BODY()` 展开的三块宏；`INCLASS_NO_PURE_DECLS` 中的 `StaticClass()` 声明；`ENHANCED_CONSTRUCTORS` 中删除的拷贝构造。
3. 实现构造函数：用 `HasAnyFlags(RF_ClassDefaultObject)` 区分 CDO 与普通实例，仅对非 CDO 初始化 `Counter`。
4. 实现 `AdvanceCounter()`：`Counter++` 并 `UE_LOG`。
5. 在 `StartupModule` 中用 `NewObject<UExD1SampleObject>(GetTransientPackage())` 创建实例，调用 `AdvanceCounter()`，验证 `StaticClass() == GetClass()`。

## 进阶任务

- 在 `UExD1SampleObject` 上添加 `UCLASS(meta=(ToolTip="演示数据对象"))` 并重新 UHT，观察生成的 `.gen.cpp` 中 meta 数据格式变化。
- 对比 `UCLASS(Abstract)` vs 无 Abstract：前者的 CDO 不能被直接实例化，影响 `GetPrivateStaticClass()` 的实现路径。
- 故意把 `.generated.h` 从头文件最后一行移到第一行，记录 UHT 报错 vs C++ 编译器报错的区别。
- 实现 `DumpProperties()`：用 `TFieldIterator<FProperty>` 遍历所有 `UPROPERTY` 并打印 Name + CPPType。

## 验收点

- [ ] 编译通过，`.generated.h` 生成到 `Intermediate/Build/.../Inc/ExD1_HelloUObject/UHT/`。
- [ ] PIE 启动日志出现 CDO 构造和实例构造的 `UE_LOG` 序列。
- [ ] 能不借助文档说出 `.generated.h` 里三个 `#define` 块各自的职责。
- [ ] `StaticClass() == GetClass()` 验证通过，能解释为什么。
- [ ] 能答出四个固定问题。

## 观察点

- UHT 的报错和 C++ 编译器的报错在 IDE 输出里来自不同进程：UHT 错误显示 "UnrealHeaderTool" 字样，C++ 错误来自 CL.exe / Clang。
- `Z_Construct_UClass_UExD1SampleObject_NoRegister()` 使用惰性初始化模式——只有第一次被调用时才真正执行注册，与 UE 模块按需加载设计一致。
- `UPROPERTY(TObjectPtr<UObject>)` 与裸 `UObject*` 在生成的 `.gen.cpp` 里的 `FObjectPropertyDesc` 完全相同——`TObjectPtr` 的差别体现在运行时（access barrier），不在 UHT 元数据层。

## 常见坑

- **`#include "ExD1_HelloUObject.generated.h"` 放错位置**：UHT 要求它是最后一个 include；放在前面时 UHT 报 `'...generated.h' must be the last include`，这是 UHT 阶段错误，与编译器无关。
- **模块 API 宏拼写错误**：UBT 生成的 API 宏规则是模块名转大写 + `_API`，`ExD1_HelloUObject` → `EXD1_HELLOUOBJECT_API`。
- **直接裸 `new UExD1SampleObject()`**：不在 `GUObjectArray` 中，不受 GC 管理，大多数 UE 框架函数访问时会断言失败。

## 提示

- 生成文件路径：`<Project>/Intermediate/Build/<Platform>/<Target>/Inc/<ModuleName>/UHT/`。
- `UE_LOG(LogExD1, Log, TEXT("..."))` 在 Output Log 用 `LogExD1` 过滤查看。
- `UExD1SampleObject::StaticClass()->GetDefaultObject<UExD1SampleObject>()` 可以安全获取 CDO。

## 复盘问题

1. 真正开始执行的时刻？（`UExD1SampleObject::StaticClass()` 的元数据第一次注册发生在 Module 的 `StartupModule` 之后，`Z_Construct_UClass_UExD1SampleObject` 被 `FClassRegistrationInfo` 的惰性初始化触发，发生在 GameThread，Module 加载阶段。）
2. 谁负责这对象的生命周期？（GC 管理；`NewObject<UExD1SampleObject>(GetTransientPackage())` 创建后若无 `UPROPERTY` 引用且无 `AddToRoot()`，下次 GC 即可回收。）
3. 涉及哪些命名线程？（`NewObject` 只能在 GameThread 调用；UObject 分配、反射查找、`GUObjectArray` 写入均假设在 GameThread，Worker thread 上调用会触发断言。）
4. 这对 GC 如何可见？（`Counter` 是 `int32`，无 UObject 引用，GC 不追踪它。若 `UExD1SampleObject` 没有被 `UPROPERTY` 引用且未 `AddToRoot()`，则 GC 可回收该实例。）
5. [本题专属] CDO 与实例化对象在反射元数据上是否共享 `UClass*`？（是的——`StaticClass()` 返回的 `UClass*` 是全局单例；CDO 和所有实例都通过 `ClassPrivate` 字段指向同一个 `UClass` 对象。）

## 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/Object.h:93`（`UObject` 基类 `UCLASS` + `GENERATED_BODY()`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h`（`GENERATED_BODY`、`UCLASS`、`UPROPERTY`、`UFUNCTION` 宏定义）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h:1889`（`NewObject<T>` 模板重载）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:4373`（`UClass::GetDefaultObject()` CDO 懒初始化入口）
