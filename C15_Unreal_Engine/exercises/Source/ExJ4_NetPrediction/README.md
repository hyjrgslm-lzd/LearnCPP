> 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-4

## 目标

阅读 `UCharacterMovementComponent` 中客户端预测（client prediction）的骨架，理解 `FSavedMove_Character`、Server RPC（`ServerMovePacked`）、Client RPC（`ClientAdjustPosition`）三者协作的时序；在自己的 Actor 里实现三类 RPC（`Server, Reliable` / `Client, Reliable` / `NetMulticast, Unreliable`）的最小骨架，验证编译正确性并能画出 client input tick 与 server ack 的时序图。

## 前置理解

- 已完成 ExJ1/ExJ2，理解 Server RPC / Client RPC 机制和 `_Implementation` 后缀约定
- 已读模块 K：`APlayerController` → `APawn` → `UPawnMovementComponent` 的层级关系
- 基本的刚体运动概念（位置、速度、加速度）

## 必做任务

1. 声明并实现 `UFUNCTION(Server, Reliable) void ServerMove(FVector_NetQuantize Loc)`：`_Implementation` 在服务端执行，`check(HasAuthority())`，打印收到的位置，调用 `ClientCorrect` 发回校正
2. 声明并实现 `UFUNCTION(Client, Reliable) void ClientCorrect(FVector Correct)`：`_Implementation` 在客户端执行，打印校正位置，将 `PredictedLocation` 重置为 `Correct`
3. 声明并实现 `UFUNCTION(NetMulticast, Unreliable) void MulticastExplode()`：`_Implementation` 在服务端和所有客户端均执行，打印 `GetLocalRole()`
4. 客户端 `Tick` 每 0.5 秒调用 `ServerMove`（模拟输入上报），服务端每 2 秒调用 `MulticastExplode`
5. **阅读任务**：阅读 `CharacterMovementComponent.h` 的 prediction 注释（2350-2371行）和调用链（2373-2379行），画出 ASCII 时序图（见下方模板）

## 时序图模板（必须手动填写）

```
时间轴 →

Client  | T1:输入↑ | 本地预测=P1 | T2:输入↑ | 本地预测=P2 | 收到Ack(T1)→确认P1 | 收到Adjust→回滚+重播
        |  ServerMove(T1,input)→→→→↓
Server  |              收到T1 | 服务端模拟=S1 | 比较P1≈S1?
        |              若|P1-S1|>阈值 → ClientAdjustPosition(S1)→→→→→→→→→→→→→→→→→↑

[填写: 每个箭头对应的 RPC 名称和方向]
[填写: SavedMoves 队列在哪个时刻清除]
[填写: 重播历史输入发生在哪个时刻]
```

## 进阶任务

- 阅读 `CharacterMovementReplication.h:128` 的 `ClientFillNetworkMoveData`，理解客户端如何把当前帧输入打包到 Server RPC 参数中
- 思考"为什么 `FSavedMove_Character` 用 `TSharedPtr` 管理而不是 UObject"：它是每帧临时分配、引用计数管理的 C++ 对象，不受 GC 追踪（对应模块 C 的 use 阶梯）
- 在 PIE Console 输入 `p.NetShowCorrections 1`，在 debug 视图中观察客户端被 server 校正时的橡皮筋效果（红色箭头）
- 设计自己的"最小预测结构"（不要求实现）：为一个飞船 Actor 实现预测，需要保存哪些状态？Server RPC 传递什么参数？写下伪代码设计

## 验收点

- [ ] 三种 RPC 骨架编译通过，`.generated.h` 中生成对应 thunk 函数声明
- [ ] PIE 中客户端每 0.5 秒打印 `ServerMove` 调用日志，服务端打印 `_Implementation` 收到日志
- [ ] `ClientCorrect_Implementation` 只在客户端打印，服务端不打印
- [ ] `MulticastExplode_Implementation` 在服务端（`Role=3`）和客户端均打印
- [ ] 能不看源码，用自己的话复述"客户端预测 + 服务端校正"的完整时序（5 步以内）
- [ ] 时序图正确标出 Server RPC（Client→Server）和 Client RPC（Server→Client）的方向
- [ ] 能解释 `FSavedMovePtr` 用 `TSharedPtr` 而非裸指针或 `UPROPERTY UObject*` 的原因

## 观察点

- `FVector_NetQuantize`：UE 专用压缩位置类型，内部是 `FVector` 子类，传输时对每个分量做量化压缩（对应 J3 的 NetSerialize 知识）
- 预测的代价：客户端与服务端各独立模拟一次移动，消耗双份 CPU；校正时需要重播 N 帧历史输入，延迟越高代价越大
- `ROLE_AutonomousProxy` 是预测的角色基础：只有本地玩家控制的 Pawn 才做 prediction；其他玩家的 Pawn（`ROLE_SimulatedProxy`）只做 interpolation（在两个服务端快照之间插值），不做 prediction

## 常见坑

- 不要尝试手写完整 CMC：`UCharacterMovementComponent` 有数千行处理台阶、斜面、网络时间同步等边界情况，本题只读主干时序
- `NetMulticast, Unreliable` 在 Standalone 模式（单人 PIE）下仍会在本地执行，但不通过网络发送；必须在 PIE 多人模式下才能验证广播行为
- Server RPC 只能从有 `ROLE_AutonomousProxy` 角色（拥有该 Actor 的客户端）调用，从 `ROLE_SimulatedProxy` 一侧调用会静默失败

## 提示

- PIE Console 常用命令：`p.NetShowCorrections 1`（显示校正箭头）、`Net.PacketDropRate 0.2`（模拟 20% 丢包）
- 查看 `CharacterMovementComponent.h`（2350行起）只需关注 `ServerMovePacked`、`ClientAdjustPosition`、`FSavedMovePtr` 三个关键词

## 复盘问题（四固定问题 + 本题专属）

1. **真正开始执行的时刻？** 客户端预测在 `TickComponent` 的哪一行开始？`ServerMove_Implementation` 在何时/何线程执行？`ClientCorrect_Implementation` 在何时执行并重播历史输入？
2. **谁负责这些对象的生命周期？** `FSavedMovePtr`（`TSharedPtr<FSavedMove_Character>`）由谁拥有？Server ack 到达时发生什么？与 `UPROPERTY UObject*` 的生命周期管理方式有何不同？
3. **涉及哪些命名线程？** CMC tick、RPC 发送/接收、历史输入重播各在哪条线程？UDP 包的底层收发在哪里？
4. **涉及 UObject 时怎么对 GC 可见？** `UCharacterMovementComponent` 本身如何对 GC 可见？`FSavedMove_Character` 不是 UObject，为何不需要 GC 追踪？
5. **本题专属：** "客户端预测 + 服务端校正"与"客户端插值（SimulatedProxy）"的本质区别是什么？两种方案各适用于什么场景？

## 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h`：prediction 注释（2350-2371行）、`CallServerMovePacked`（2379行）、`ServerMoveHandleClientError`（2387行）、`FSavedMovePtr` typedef（117行）
- `Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementReplication.h`：`FSavedMove_Character` 前向声明（17行）、`ClientFillNetworkMoveData`（128行）、`ClientAdjustPosition` 注释（252行）
- `Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h`：`Server`, `Reliable`, `NetMulticast` 宏定义
