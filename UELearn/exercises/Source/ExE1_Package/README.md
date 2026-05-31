> 对应章节: ../../../07-模块E-资产与加载.md §练习 E-1

## 目标

通过手写 `UObject` 子类并用 `FMemoryWriter` / `FMemoryReader` 做往返序列化，直观体验 `FArchive` 的"单代码双向"设计；通过 `GetOutermost()` 观察 `UPackage` 作为 UObject 容器的角色。

## 前置理解

- 已完成模块 D，能定义 `UCLASS` + `UPROPERTY` 并用 `NewObject<T>` 创建实例。
- 知道 `FArchive` 是 UE 序列化体系的基类，`FMemoryWriter` / `FMemoryReader` 是最简单的内存子类。
- 知道 `.uasset` 文件在内存中对应一个 `UPackage` 对象。

## 必做任务

1. 将 `RunSerializeDemo()` 添加到模块的 `StartupModule` 或一个控制台命令回调里，让它在 PIE 启动时执行。
2. 完成 `TODO [必做] 1`：创建 `Clone`，用 `FMemoryReader` 反序列化，打印 `Health` / `CharName` / `Speed` 字段对比原始值，确认往返一致。
3. 观察并记录：
   - `Writer.IsSaving() == true`，`Writer.IsLoading() == false`
   - `Reader.IsSaving() == false`，`Reader.IsLoading() == true`
4. 调用 `MyObj->GetOutermost()`，观察返回的 `UPackage*` 的 `GetName()`——应为 `/Engine/Transient`（临时 Package）。解释为何不指定 Outer 的 `NewObject` 对象会挂在 TransientPackage。
5. 记录序列化字节数（`Buffer.Num()`），与你预期的"3 个简单字段大小"对比，解释 Tagged Serialization 的额外开销来自哪里。

## 进阶任务

- 重写 `UExE1Serializable::Serialize`，加入不带 `UPROPERTY` 的 `int32 InternalCounter` 的 raw 序列化（`Ar << InternalCounter`），观察往返是否正确，理解 Tagged vs Untagged 差异。
- 用 `CreatePackage(TEXT("/Game/ExE1TestPkg"))` 创建具名 Package，以其为 Outer 创建对象，再调用 `UPackage::SavePackage`（Editor-only API），用十六进制查看器找到文件头魔数 `0x9E2A83C1`。
- 了解 `FLinkerLoad`：`GetLinker()` 只对从磁盘加载的 Package 返回非 null，临时 Package 无 Linker。

## 验收点

- [ ] 编译通过，Output Log 出现序列化字节数日志
- [ ] `Writer.IsSaving() == true`，`Reader.IsLoading() == true` 均有日志证明
- [ ] 往返序列化后 `Clone` 字段与原始值完全一致
- [ ] 能用 `GetOutermost()` 取到 `UPackage*` 并说清楚为何是 TransientPackage
- [ ] 能答出四个固定问题

## 观察点

- **FArchive 双向设计**：同一份 `Serialize(FArchive& Ar)` 代码靠 `Ar.IsSaving()` / `Ar.IsLoading()` 分支，在逻辑对称的结构上只写一遍，避免写/读代码不对称的 offset 漂移 bug。
- **UPackage 的容器角色**：每个 `.uasset` 文件对应内存里一个 `UPackage`，`NewObject<T>()` 不指定 Outer 时挂在 `GetTransientPackage()`——这是内存资产与磁盘资产的关键区别。
- **`FMemoryWriter` / `FMemoryReader` 不拥有 Buffer 所有权**：只持有引用，Buffer 生命周期由调用方管理（RAII 非 UObject 对象的典型模式）。

## 常见坑

- **`Serialize` 里忘调 `Super::Serialize(Ar)`**：所有 `UPROPERTY` 字段不会被序列化，但手动字段还在，结果是部分往返失败且无报错。
- **`FMemoryWriter` 的 `bIsPersistent` 参数**：控制 FArchive 是否保留 Buffer 不析构，对临时往返测试为 `true` 即可，但要确保 Buffer 生命周期长于 Writer。
- **`NewObject` 不保活 + GC**：临时创建的 `UObject*` 若不加 `AddToRoot()` 或 `UPROPERTY` 保活，GC 可能在几个 tick 内回收。骨架代码已用 `AddToRoot` / `RemoveFromRoot` 演示正确做法。
- **`UPackage::SavePackage` 是 Editor-only API**：Shipping 构建不可用，只在进阶任务中试验。

## 提示

- `FArchive` 的 `<<` 运算符委托到 `Serialize` 重载，可用于基础类型：`Ar << MyInt32; Ar << MyFString;`。
- 观察 `Buffer.Num()` 的字节数：Tagged Serialization 每个字段带 FName tag + 类型 tag，开销比裸字节大得多。

## 复盘问题

1. `Clone->Serialize(Reader)` 在哪条线程的哪个调用栈上开始执行？是否有异步路径？
2. `Clone` 指针由谁负责生存？存在普通 C++ 局部变量里 GC 下次运行时会发生什么？怎么修复？
3. `Obj->Serialize(Ar)` 完全在 GameThread 上同步完成；`LoadPackage` 则在哪些线程上运行？
4. 创建的 `Clone` 对象若没有任何 `UPROPERTY` 指向它、也没有 `AddToRoot()`，说出让 GC 保留它的三种方法。
5. **[本题专属]** `FArchive::IsSaving()` 和 `IsLoading()` 能同时为 `true` 吗？为什么？

## 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/Package.h`（`UPackage` 定义，`Save` / `SavePackage`，`GetLinker()`）
- `Engine/Source/Runtime/CoreUObject/Public/Serialization/Archive.h`（`FArchive` 基类，`IsSaving` / `IsLoading`，`<<` 运算符）
- `Engine/Source/Runtime/Core/Public/Serialization/MemoryWriter.h`（`FMemoryWriter`）
- `Engine/Source/Runtime/Core/Public/Serialization/MemoryReader.h`（`FMemoryReader`）
