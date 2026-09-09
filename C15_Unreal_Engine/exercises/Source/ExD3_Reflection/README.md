> 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-3

## 目标

用代码遍历 `UClass::PropertyLink` 链，按 `FProperty` 子类型分发，dump 每个 `UPROPERTY` 的名字、类型、内存偏移和标志。理解 UE 反射系统如何把 `UPROPERTY` 宏变成运行时可查询的元数据。

## 前置理解

- 已完成 ExD1/ExD2，理解 UHT 在 `.gen.cpp` 里生成的 `FObjectPropertyDesc`/`FIntPropertyDesc` 等描述符，以及它们如何在 Module 加载时注册到 `UClass`。
- 知道 `UClass` 继承自 `UStruct`，`PropertyLink` 是 `UStruct` 的字段（`Class.h:530`）。
- 了解 `FProperty` 是所有属性描述符的基类（`UnrealType.h`），`FBoolProperty`、`FIntProperty`、`FObjectProperty`、`FArrayProperty` 是其子类。
- **关键**: `CastField<T>` 用于 `FField` 体系（`FProperty` 族），`Cast<T>` 用于 `UObject` 体系，两者不可互换。

## 必做任务

1. 取消 `UExD3ReflectTarget` 头文件中所有 `UPROPERTY` 的注释，触发 UHT 生成各字段的 `FProperty` 描述符。
2. 实现 `ExD3_DumpClassPropertiesManual()`：手动遍历 `PropertyLink` 链，用 `CastField<T>` 按类型分发，打印 Name + TypeName + Offset + Flags。观察 `HiddenField` 是否出现。
3. 实现 `ExD3_DumpClassPropertiesIterator()`：用 `TFieldIterator<FProperty>` 改写，对比与手动遍历的覆盖范围差异（含/不含基类属性）。
4. 取消注释 `OnPropertyChanged` 的 `UFUNCTION`，用 `FindFunctionByName` + `ProcessEvent` 通过反射调用它。
5. 实现 `ExD3_WriteAndReadViaReflection()`：用 `FObjectProperty::SetObjectPropertyValue_InContainer` 写入 `ChildRef` 字段，再用 `GetObjectPropertyValue_InContainer` 验证读取正确。

## 进阶任务

- 遍历 `UObject` 基类的 `PropertyLink`，统计 `UObject` 自身定义了多少个 `UPROPERTY`（通常为 0 或极少）。
- 实现简单"JSON 序列化器"：遍历 `PropertyLink`，把所有基础类型字段（bool/int32/float/FString）序列化为 JSON 字符串，验证反射驱动序列化的可行性。
- 研究 `FProperty::EPropertyFlags`（`ObjectMacros.h`），找出 `CPF_Net`、`CPF_Edit`、`CPF_BlueprintVisible` 的值，理解 `UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite)` 如何映射到这些标志位。

## 验收点

- [ ] 编译通过，`DumpClassProperties` 输出在 PIE Output Log 可见。
- [ ] 能徒手画出 `UClass::PropertyLink` 链的内存结构（单向链表，每个节点是 `FProperty*`，通过 `PropertyLinkNext` 连接）。
- [ ] 能说出 `HiddenField` 不出现在 PropertyLink 中的根本原因。
- [ ] 能区分 `CastField<T>` 和 `Cast<T>` 的适用场景。
- [ ] 通过 `SetObjectPropertyValue_InContainer` 写入字段，`GetObjectPropertyValue_InContainer` 验证读取正确。
- [ ] 能答出四个固定问题。

## 观察点

**反射可见性图**（必画）：
```
UExD3ReflectTarget（UCLASS）
├── UPROPERTY 字段（UHT 可见 / GC 可见 / 编辑器可见）
│   ├── bActive   → FBoolProperty    (offset=X, flags=CPF_Edit|CPF_BlueprintVisible)
│   ├── Count     → FIntProperty
│   ├── Weight    → FFloatProperty
│   ├── Label     → FStrProperty
│   ├── ChildRef  → FObjectProperty  (PropertyClass=UObject, GC 通过此 prop 遍历引用)
│   └── Numbers   → FArrayProperty   (Inner=FIntProperty)
└── 普通 C++ 字段（UHT 不可见 / GC 不可见）
    └── HiddenField → 仅 C++ 编译器可见
```

**PropertyLink vs RefLink**（`Class.h:529-536`）：
- `PropertyLink`：所有 `UPROPERTY` 字段的链表（最派生到基类）
- `RefLink`：仅含持有 UObject 引用的属性（GC 可达性分析主要遍历 `RefLink`，不是全量 `PropertyLink`，这是性能优化）

## 常见坑

- **用 `Cast<FProperty>` 代替 `CastField<FProperty>`**：`FProperty` 从 UE4.25 起不再继承 `UObject`，`Cast<>` 不适用，编译失败。
- **遍历时修改 PropertyLink 链**：遍历期间不要调用 `AddProperty` 等修改链表的操作，会导致迭代器失效。
- **`GetOffset_ForInternal()` 的用途**：返回字段相对对象起始地址的字节偏移，`(uint8*)Object + Offset` 即字段地址——这正是 GC 遍历和序列化系统的底层机制。

## 提示

- `TFieldIterator<FProperty>(Class, EFieldIterationFlags::IncludeSuper)` 同时遍历继承自基类的属性；`EFieldIterationFlags::ExcludeSuper` 只遍历本类定义的属性。
- `FindPropertyByName(TEXT("ChildRef"))` 比手动遍历更方便，适合精确查找单个字段。
- `UFunction` 可通过 `StaticClass()->FindFunctionByName(TEXT("OnPropertyChanged"))` 找到，`ProcessEvent(Func, nullptr)` 调用它（`nullptr` 表示无参数缓冲区）。

## 复盘问题

1. 真正开始执行的时刻？（`DumpClassProperties` 在 GameThread 的 `StartupModule` 调用时；此时 `UClass` 的 PropertyLink 已被 UHT 生成的 `.gen.cpp` 注册完毕。）
2. 谁负责这对象的生命周期？（GC 管理；`NewObject<UExD3ReflectTarget>(GetTransientPackage())` 后若无 `UPROPERTY` 引用也无 `AddToRoot()`，下次 GC 即回收。）
3. 涉及哪些命名线程？（纯 GameThread；反射查询是只读的，不涉及 RenderThread 或 RHIThread。）
4. 这对 GC 如何可见？（`ChildRef` 字段的 `FObjectProperty` 节点注册到 `UClass::RefLink`；GC 遍历 `RefLink` 时读取 `ChildRef` 字段的指针值，调用 `UE::GC::MarkAsReachable` 标记目标对象。）
5. [本题专属] 为什么 GC 用 `RefLink` 而不是 `PropertyLink` 做可达性分析？（性能：只有包含 UObject 引用的属性才需要追踪；`int32`/`float`/`FString` 字段无法持有 UObject 引用，跳过它们可以把 GC 遍历耗时降低一个数量级。）

## 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:211`（`FProperty::PropertyLinkNext`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h:2542,3086,3701`（`FBoolProperty`、`FObjectProperty`、`FArrayProperty`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h:529`（`UStruct::PropertyLink`/`RefLink` 字段）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/FieldIterator.h`（`TFieldIterator` 实现）
