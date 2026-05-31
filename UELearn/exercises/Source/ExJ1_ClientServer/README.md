> 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-1

## 目标

搭建可在 PIE 多人模式下运行的最小 replicated Actor，亲眼观察服务端 SpawnActor 如何在客户端自动出现，以及服务端移动如何通过 `ReplicatedMovement` 同步到客户端，同时理解三种 Role 在不同端的取值。

## 前置理解

- 已完成模块 K（`AActor` 生命周期钩子：`BeginPlay` / `Tick` / `EndPlay`）
- 知道 `SpawnActor<T>` 在 `UWorld` 上的调用方式
- 知道 `UWorld::GetNetMode()` 返回 `ENetMode`

## PIE 多人配置（必读，不得跳过）

1. 点击 Unreal Editor 顶部 **Play** 按钮旁的 **下拉箭头** → **Advanced Settings**
2. 设置 **Number of Players = 2**
3. 设置 **Net Mode = Play As Dedicated Server**（推荐）
4. 点击 **Play**
5. Editor 打开两个视口：一个 Dedicated Server 窗口（无渲染画面），一个 Client 视口
6. 在 **Output Log** 面板的下拉菜单中切换 **"Server"** / **"Client 1"** 分别查看日志

> 若选 **Play As Listen Server**，第一个视口既是服务端也是本地客户端，差异见进阶任务。

## 必做任务

1. 确认 `.Build.cs` 中 `NetCore` 已添加到依赖
2. 在 `GameMode::BeginPlay` 里（GameMode 只在服务端存在）调用 `SpawnActor<AExJ1ServerSpawned>`
3. PIE 启动后对照 Output Log Server / Client 两侧的 Role 日志：
   - Server: `Role=3`（`ROLE_Authority`），`HasAuth=1`
   - Client: `Role=1`（`ROLE_SimulatedProxy`），`HasAuth=0`
4. 取消 `Tick` 里的注释块，服务端每帧沿 X 轴移动，客户端 Tick 打印 `GetActorLocation().X`
5. 观察客户端位置是否跟随服务端变化（依赖 `AActor::ReplicatedMovement` 默认 replication）

## 进阶任务

- 将 `NetUpdateFrequency = 2.0f`，观察客户端位置更新卡顿，理解 NetDriver 更新率控制
- 切换 Net Mode = Play As Listen Server，再观察 Role 打印值
- 在 `Actor.cpp` 第 2014 行找到 `DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(AActor, ReplicatedMovement, ...)` 验证运动同步是内置的

## 验收点

- [ ] PIE 启动时 Output Log 可区分 Server / Client 两侧，服务端打印 `Role=3`，客户端打印 `Role=1`
- [ ] 服务端 SpawnActor 后客户端无需任何代码即可看到该 Actor 出现（Actor Channel 自动创建）
- [ ] 服务端 SetActorLocation 后客户端位置在 `1/NetUpdateFrequency` 秒内更新
- [ ] 能解释为什么不需要手写 `DOREPLIFETIME` 就能看到位置同步

## 观察点

- `bReplicates = true` 使 Actor 加入 NetDriver 的复制列表；`SetReplicates(true)` 应在 World 完全初始化后调用
- `HasAuthority()` 是判断 Server/Client 的标准方式，等价于 `GetLocalRole() == ROLE_Authority`
- 客户端对 `ROLE_SimulatedProxy` Actor 调用 `SetActorLocation` → 下次 replication tick 被服务端值覆盖
- `GetNetMode()` 在 Standalone 模式返回 `NM_Standalone`，此时 `bReplicates` 虽为 true 但 NetDriver 不存在，RPC 静默失败

## 常见坑

- **PIE 只启动一个窗口**：检查 Advanced Settings → Number of Players 是否确实设置为 2
- **在客户端 SpawnActor**：客户端无 Authority，SpawnActor 成功但该对象只存在于本地，不会被同步
- **`bReplicates` 在 BeginPlay 后才设为 true**：建议在构造函数里设置；BeginPlay 后通过 `SetReplicates(true)` 也可行但时机较晚

## 提示

- 在 Output Log 面板左上角的下拉菜单里可以选择查看 "Server"、"Client 1" 等独立日志流
- 用 `p.NetShowCorrections 1`（console 命令）可在视口里可视化服务端校正位移（红色箭头）

## 复盘问题（四固定问题 + 本题专属）

1. **执行真正开始时刻？** 服务端 `SpawnActor` 触发 Actor 构造和 `BeginPlay`（GameThread）；NetDriver 在每帧 `TickFlush`（GameThread）的 `ServerReplicateActors` 中将 dirty properties 发往客户端；客户端在 `TickDispatch`（GameThread）收到包后更新 Actor 状态
2. **生命周期拥有者？** `UNetDriver` 由 `UWorld` 持有（UPROPERTY）；`UNetConnection` 由 NetDriver 的 `ServerConnections` 数组持有（UPROPERTY）；`UActorChannel` 由 `UNetConnection` 的 Channels 数组持有（UPROPERTY）；replicated Actor 由 `UWorld`/`ULevel` 的 Actors 数组持有
3. **涉及哪些 Named Thread？** 主要是 **GameThread**（`TickDispatch` / `TickFlush` / RPC 执行）；包的底层收发可能在 **NetThread**（`FReceiveThreadRunnable`）；渲染无关
4. **涉及 UObject 时怎么对 GC 可见？** 整条 World → NetDriver → Connection → Channel → Actor 链路均通过 `UPROPERTY` 实现 GC 可见；`bReplicates` 本身是非 UPROPERTY 的 C++ 位字段
5. **[本题专属]** 为什么服务端 SpawnActor 后客户端无需任何代码即可看到该 Actor？答：NetDriver 在 `ServerReplicateActors` 时发现新的相关 Actor，在对应客户端的 `UNetConnection` 上自动创建 `UActorChannel`，通过该 Channel 将 Actor 的初始状态序列化并发送；客户端收到后自动在本地 World 中 Spawn 该 Actor 的副本

## 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h`（`bReplicates` 553行，`HasAuthority()` 1941行，`SetReplicates` 722行）
- `Engine/Source/Runtime/Engine/Private/Actor.cpp`（`GetLifetimeReplicatedProps` 2014行，`DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(AActor, ReplicatedMovement, ...)`）
- `Engine/Source/Runtime/Engine/Classes/Engine/NetDriver.h`（三层结构注释 38–56行）
- `Engine/Source/Runtime/Engine/Classes/Engine/Channel.h`（`UChannel` 62行，`Connection` UPROPERTY 68行）
