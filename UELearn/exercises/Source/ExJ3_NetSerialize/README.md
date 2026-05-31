> 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-3

## 目标

为自定义 struct 实现 `NetSerialize` 特化，演示把两个 `float` 压缩到各 16bit 整数进行传输，理解 `FArchive::IsSaving()` / `IsLoading()` 在网络路径下的语义（对应模块 E 的 FArchive 双向序列化知识），并通过 `TStructOpsTypeTraits` 告知引擎使用自定义序列化器。

## 前置理解

- 已完成 ExJ2_Replication，理解 `DOREPLIFETIME` 与 `UPROPERTY(Replicated)` 的关系
- 已读模块 E：`FArchive` 的双向序列化惯例（`IsSaving` = 写入方向，`IsLoading` = 读取方向）
- 已读模块 D：`USTRUCT` + `GENERATED_BODY()` 的基本写法

## 必做任务

1. 声明 `USTRUCT() struct FExJ3CompressedData { GENERATED_BODY() ... }`，含 `float X; float Y;`
2. 实现 `bool NetSerialize(FArchive& Ar, UPackageMap*, bool& bOutSuccess)`：发送端用 `Ar.IsSaving()` 将 float 量化为 `int16`（乘以 100 取整），接收端用 `Ar.IsLoading()` 反量化（除以 100）
3. 特化 `TStructOpsTypeTraits<FExJ3CompressedData>`，设置 `enum { WithNetSerializer = true }`
4. 在 `AExJ3NetSerializeActor` 里声明 `UPROPERTY(Replicated) FExJ3CompressedData CompressedPos;`
5. 实现 `GetLifetimeReplicatedProps`，用 `DOREPLIFETIME` 注册 `CompressedPos`
6. 服务端每 1 秒更新 `CompressedPos`，客户端打印收到的值并验证量化误差 `< 0.02`

## 进阶任务

- 把精度从 100 提升到 1000（乘以 1000 取整），验证误差降低到 `< 0.002`，同时验证 `int16` 的值域限制（`[-32.768, 32.767]`）
- 对比不使用 `WithNetSerializer=true` 时的行为：去掉特化，观察引擎退回到逐字段默认序列化时的日志和带宽变化
- 为 `FExJ3CompressedData` 增加一个 `float Z`，验证三个 float 都经过 NetSerialize 压缩

## 验收点

- [ ] `WithNetSerializer = true` 特化编译通过，UHT 不报错
- [ ] PIE（Number of Players=2）中服务端发送 `X=12.345`，客户端收到 `X=12.35`（误差 `< 0.02`）
- [ ] 去掉 `MarkItemDirty` 后观察到客户端不更新（演示 dirty tracking 依赖手动标记）
- [ ] 能解释 `FArchive::IsSaving()` 在网络路径下对应哪端，以及与模块 E 的资产序列化有何相同之处

## 观察点

- `NetSerialize` 的签名 `bool NetSerialize(FArchive& Ar, UPackageMap*, bool& bOutSuccess)` 与模块 E 的资产序列化共享相同的 `FArchive` 基类——`IsSaving()` 在发送端为真，`IsLoading()` 在接收端为真，同一份代码跑两个方向
- `TStructOpsTypeTraits` 是 UE 的 policy-based 设计：通过 `enum` 编译期常量告知引擎"这个 struct 有哪些特殊能力"，而无需继承或虚函数
- 量化压缩的本质是精度换带宽：`float`（4 字节）→ `int16`（2 字节），带宽减半，精度损失约 `1/scale`

## 常见坑

- `NetSerialize` 函数不加 `UFUNCTION` 宏——正确，`NetSerialize` 是普通成员函数，不是 `UFUNCTION`
- 忘记特化 `TStructOpsTypeTraits` → 引擎不知道有自定义序列化器，退回默认逐字段序列化，`NetSerialize` 不被调用
- 量化时未处理 `int16` 溢出 → 若 float 绝对值 `> 327.67`（`32767/100`），强转 `int16` 会溢出，接收端得到错误值；用 `FMath::Clamp` 限制范围
- 在 `NetSerialize` 里用 `Ar.IsLoading()` 时忘记先声明变量再 `Ar << var` → 未初始化变量导致解量化后得到垃圾值

## 提示

- `FArchive` 重载 `operator<<`，对不同类型自动分发读写方向；`int16` 的 `<<` 在 saving 时写入，在 loading 时读出
- 验证 `WithNetSerializer` 是否生效：在 `NetSerialize` 函数开头加 `UE_LOG`，观察 PIE 中是否被调用

## 复盘问题（四固定问题 + 本题专属）

1. **真正开始执行的时刻？** `NetDeltaSerialize` / `NetSerialize` 在什么时机被调用？在哪条线程？
2. **谁负责这些对象的生命周期？** `FExJ3CompressedData` 作为 Actor 的成员变量，生命周期与谁同步？传输中的临时 `FArchive` 对象何时析构？
3. **涉及哪些命名线程？** `NetSerialize` 调用发生在 `GameThread` 还是其他线程？
4. **涉及 UObject 时怎么对 GC 可见？** `FExJ3CompressedData` 是 `USTRUCT` 而非 `UCLASS`，GC 如何通过包含它的 Actor 字段追踪它内部的 `UPROPERTY`？
5. **本题专属：** `FArchive::IsSaving()` 在网络序列化中对应"发送端还是接收端"？与模块 E 中资产加载时 `IsLoading()` 对应"从磁盘读"的语义是否完全一致？

## 对应官方参考

- `Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectMacros.h`：`USTRUCT`、`GENERATED_BODY`
- `Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h`：`DOREPLIFETIME`（259行）
- `Engine/Source/Runtime/Net/Core/Classes/Net/Serialization/FastArraySerializer.h`：`NetDeltaSerialize` 接口和 6 步使用说明（62-117行）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/CoreNet.h`：`FFieldNetCache`（72行）
