> 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-2

## 目标

手写完整的 replicated property（含 `DOREPLIFETIME`），实现 RepNotify 回调，并各写一个 Server / Client / NetMulticast RPC，深刻理解三向 RPC 的触发条件与执行端，以及 `_Implementation` 后缀的 UHT 代码生成机制。

## 前置理解

- 已完成 ExJ1_ClientServer，理解 PIE 多人模式和 Role 三态（`ROLE_Authority` / `ROLE_AutonomousProxy` / `ROLE_SimulatedProxy`）
- 已读模块 D：`UPROPERTY` 宏与 UHT 代码生成，`.generated.h` 中的声明展开
- 已读模块 K：`AActor` 生命周期钩子（`BeginPlay` / `Tick`）

## 必做任务

1. 新建 `AExJ2ReplicatedActor`，`SetReplicates(true)`，在构造器里设置 `Health = 100.f`
2. 声明 `UPROPERTY(ReplicatedUsing=OnRep_Health) float Health;` + `UFUNCTION() void OnRep_Health();`，实现 `OnRep_Health` 打印新值
3. 实现 `GetLifetimeReplicatedProps`，调用 `Super::GetLifetimeReplicatedProps`，再 `DOREPLIFETIME(AExJ2ReplicatedActor, Health)`
4. 实现 `Server, Reliable` RPC `ServerRequestDamage`，在 `_Implementation` 里 `check(HasAuthority())` 并扣血
5. 实现 `Client, Reliable` RPC `ClientNotifyDead`，在 `_Implementation` 里打印日志
6. 实现 `NetMulticast, Unreliable` RPC `MulticastExplode`，在 `_Implementation` 里打印 `GetLocalRole()`
7. 在服务端 `Tick` 每隔 2 秒减少 `Health`，`Health` 降至 0 时依次调用 `ClientNotifyDead()` 和 `MulticastExplode()`

## 进阶任务

- 改用 `DOREPLIFETIME_CONDITION(AExJ2ReplicatedActor, Health, COND_OwnerOnly)`，观察只有 Owner 客户端能收到更新（参考 `CoreNetTypes.h:21-38`）
- 将 `MulticastExplode` 改为 `Reliable`，对比与 `Unreliable` 在模拟丢包时的行为差异（Editor Console: `Net.PacketDropRate 0.3`）
- 在 `.generated.h` 中找到 RPC thunk 函数声明，理解 `_Implementation` 是 UHT 约定而非 C++ 语言特性

## 验收点

- [ ] PIE（Number of Players=2）启动，Output Log 中服务端打印 `Role=3`，客户端打印 `Role=2`
- [ ] `OnRep_Health` 在客户端被调用，服务端日志中不出现该函数的日志
- [ ] `Server RPC` 的 `_Implementation` 里 `check(HasAuthority())` 不触发断言
- [ ] `ClientNotifyDead` 日志只在目标客户端出现，不在服务端出现
- [ ] `MulticastExplode` 日志在服务端（`Role=3`）和客户端（`Role=2`）均出现
- [ ] 能在 `.generated.h` 中找到 RPC 相关的 thunk 函数声明

## 观察点

- `OnRep_Health` 回调只在接收端（客户端）调用，与函数的执行端无关——这是 `ReplicatedUsing` 的语义
- `_Implementation` 函数的调用者是 UHT 生成的 thunk，不是你的业务代码；调用 `ServerRequestDamage(10.f)` 时实际执行的是 thunk，负责序列化参数并通过 `UNetConnection` 发送
- `NetMulticast` 的 `Unreliable` 版本在网络拥堵时可能不送达；对于游戏内爆炸音效/粒子这类"丢了也无所谓"的效果，`Unreliable` 是正确选择

## 常见坑

- 忘记调用 `Super::GetLifetimeReplicatedProps` → 基类的 `ReplicatedMovement` 不会注册，位置同步失效
- 声明了 `UPROPERTY(Replicated)` 但忘写 `DOREPLIFETIME` → 字段永远不同步，部分 UE 版本会打印运行时警告
- 把 `Client RPC` 当广播用 → `Client` 只发往 Owner 对应的客户端，广播用 `NetMulticast`
- 在没有 Authority 的端调用 `Client/Multicast RPC` → 静默失效，不报错
- 在客户端调用 `SetActorLocation` 后被服务端覆盖 → 用 `HasAuthority()` 守卫所有状态修改

## 提示

- PIE 多人配置: Edit → Editor Preferences → Play → Number of Players = 2, Net Mode = Play As Dedicated Server
- Output Log 下拉菜单可切换查看 "Server" 或 "Client" 的独立日志
- `ROLE_Authority=3`, `ROLE_AutonomousProxy=2`（本地玩家控制的 Pawn 在其客户端上）, `ROLE_SimulatedProxy=1`（其他玩家的 Actor）

## 复盘问题（四固定问题 + 本题专属）

1. **真正开始执行的时刻？** `DOREPLIFETIME` 的注册在 `GetLifetimeReplicatedProps` 里，何时被调用？`ServerRequestDamage_Implementation` 在何时/何条线程执行？
2. **谁负责这些对象的生命周期？** `FLifetimeProperty` 数组的生命周期是什么？RPC 参数的网络包由谁管理？
3. **涉及哪些命名线程？** `GetLifetimeReplicatedProps` 调用、`ServerReplicateActors` 驱动、`OnRep_*` 回调、RPC `_Implementation` 执行各在哪条线程？
4. **涉及 UObject 时怎么对 GC 可见？** `Health` 是 `float`，GC 不追踪；若 replicated property 是 `UObject*` 类型，需要怎么处理？
5. **本题专属：** UHT 为 `ServerRequestDamage` 生成了哪些函数？`_Implementation` 后缀约定来自何处？在 `.generated.h` 里能找到哪些相关声明？

## 对应官方参考

- `Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h`：`DOREPLIFETIME`（259行）、`DOREPLIFETIME_CONDITION`（277行）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/CoreNetTypes.h`：`COND_*` 枚举（21-38行）
- `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h`：`GetLifetimeReplicatedProps`（273行）、`HasAuthority()`（1941行）
- `Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerController.h`：RPC 声明样例（2297行 `UFUNCTION(Client, Reliable)`）
