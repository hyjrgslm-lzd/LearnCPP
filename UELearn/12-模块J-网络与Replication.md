# 12 模块 J：网络与 Replication

## 模块目标

模块 J 是整套课程依赖链最长的一站：它把模块 D 的反射元数据、模块 E 的 FArchive 序列化、模块 F 的任务图线程模型、以及模块 K 的 Actor 生命周期全部汇聚到一个主题——**让同一个游戏世界的状态，安全地在服务端与多个客户端之间保持一致**。

与 C++ 标准库的异步工具（stdexec sender / UE::Tasks）不同，网络 replication 没有对应的标准类比。它的核心概念是 UE 特有的：`ROLE_Authority` 决定谁拥有真相，`UNetDriver` 驱动每帧的 actor 状态分发，`DOREPLIFETIME` 宏把反射元数据与网络同步范围绑定在一起，RPC 的 `_Implementation` 后缀机制通过 UHT 代码生成实现跨机器函数调用。

阶梯定位：**use（J1/J2）→ inspect（J2/J3/J4）→ implement-own（J3）**。

反向引用 `01-心智模型.md` §2（Reflection before types）与 §4（GC ownership 与 C++ ownership 两套并存）：DOREPLIFETIME / UPROPERTY(Replicated) / UFUNCTION(Server/Client/NetMulticast) 本质是 D 模块反射元数据在网络维度的投射；Channel/Connection/Actor 的生命周期链 + UPROPERTY GC 可见性决定了哪些对象可被网络路径安全持有。

---

## J 模块依赖链

在开始练习之前，务必显式确认以下前置模块的关键概念：

- **模块 D（UObject/反射/GC）**：`UPROPERTY(Replicated)` 与 `UPROPERTY(ReplicatedUsing=...)` 必须被 UHT 解析并在 `.generated.h` 中产生反射元数据；`GetLifetimeReplicatedProps` 在 `Actor.cpp` 里通过 `DOREPLIFETIME` 宏向 `FLifetimeProperty` 数组注册字段，NetDriver 在每帧 `ServerReplicateActors` 时查询这张表。没有 D 的反射系统，replication 无从运作。

- **模块 E（FArchive 序列化）**：UE 的网络序列化路径建立在 `FArchive` 的"同一份代码跑两个方向"惯例之上。在 `NetSerialize` 特化里，`FArchive::IsSaving()` 为真时是发送端打包，`FArchive::IsLoading()` 为真时是接收端解包——这与 E 模块里的 `UPackage` 序列化使用完全相同的基础抽象。`FastArraySerializer.h` 的 `NetDeltaSerialize` 接口也基于 `FNetDeltaSerializeInfo`，底层同样是 `FArchive` 派生。

- **模块 F（并发与任务图）**：网络 tick 在 `GameThread` 上发起（`UNetDriver::TickDispatch` / `TickFlush`），但实际的包接收解析会被投递到专用的网络线程（`FReceiveThreadRunnable`）或利用 TaskGraph worker pool 做并行解包。RPC 的 server 端执行最终回到 `GameThread`。理解 F 模块的 Named Thread 模型，才能正确回答"这段代码在哪条线程执行"。

- **模块 K（Actor/Component/World/Subsystem）**：replication 的主体是 `AActor`。`ROLE_Authority`、`ROLE_AutonomousProxy`、`ROLE_SimulatedProxy` 三种 role 是 Actor 生命周期状态的一部分，`HasAuthority()` 是 Actor 的成员函数（`Actor.h:1941`）。`UActorChannel` 是每个被复制 Actor 在 `UNetConnection` 上拥有的专属通道，Channel 的 Outer 是 Connection，Connection 的 Outer 是 NetDriver，NetDriver 由 `UWorld` 持有——这是 J 模块"Connection/Channel/Actor 三层生命周期"的具体体现。

---

## 模块完成标准

做完本模块，你至少要能稳定说清楚以下五条：

1. **`UNetDriver` — `UNetConnection` — `UChannel` 三层结构的职责划分**：Driver 管理所有连接集合，Connection 代表一个玩家端点，Channel 是连接内的逻辑子信道（Control Channel / Actor Channel / Voice Channel）。一个 replicated Actor 在每个 Connection 上有一个独立的 `UActorChannel`。

2. **`bReplicates = true` 与 `SetReplicates(true)` 的区别**：前者在 Actor 构造器里设置（构造阶段标记），后者在初始化后调用（`Actor.h:722`，内部处理 World 网络对象列表注册）。两者都导致 `RemoteRole = ROLE_SimulatedProxy`（`Actor.cpp:551`）。

3. **`DOREPLIFETIME` 宏的展开路径**：宏定义于 `Engine/Public/Net/UnrealNetwork.h:259`，展开为向 `OutLifetimeProps` 数组压入 `FLifetimeProperty` 条目。`GetLifetimeReplicatedProps` 是 `UObject` 的虚函数，`AActor::GetLifetimeReplicatedProps`（`Actor.h:273`）在此注册 `ReplicatedMovement`、`AttachmentReplication` 等基础字段，子类 override 时调用 `Super` 后追加自己的字段。

4. **RPC 三向的触发条件**：`Server` RPC 只能从拥有权限的客户端（`ROLE_AutonomousProxy`）调用，在服务端执行；`Client` RPC 只能从服务端调用，在目标客户端执行；`NetMulticast` 从服务端调用，在服务端和所有连接的客户端执行。`_Implementation` 后缀由 UHT 生成，手写逻辑放在该后缀函数里。

5. **`FFastArraySerializer` 为何优于全量同步**：经典 replication 对整个数组比较 Recent 缓冲区，任何元素变更都触发全量重传。`FFastArraySerializer` 为每个元素维护版本号（`ReplicationID` / `ReplicationKey`）和 dirty flag，每帧只序列化真正变更的元素，支持 `PostReplicatedAdd` / `PostReplicatedChange` / `PreReplicatedRemove` 精确回调（`FastArraySerializer.h:83-88`）。

---

## 网络三层结构概览

在进入练习之前，先建立 UE 网络栈的层次心智模型。

```
UWorld
  └── UNetDriver          (Game NetDriver，每 World 一个)
        ├── ServerConnections[]   (服务端侧：每连接的客户端一个 UNetConnection)
        └── ClientConnection      (客户端侧：与服务端的唯一连接)

UNetConnection
  └── Channels[]
        ├── UControlChannel   (idx 0 — 连接控制/握手)
        ├── UVoiceChannel     (idx 1 — 语音)
        └── UActorChannel     (per-Actor — Actor 状态同步 + RPC)
```

`UNetDriver` 的注释（`NetDriver.h:38-56`）明确指出三种常见 Driver 类型：Game NetDriver（主游戏流量）、Demo NetDriver（录像/回放）、Beacon NetDriver（对话/大厅流量）。本模块聚焦 Game NetDriver。

每个被 replicate 的 Actor，在每个 `UNetConnection` 上都有一个唯一的 `UActorChannel`（`NetConnection.h:64`，`FActorChannelMap` 类型定义）。Channel 在首次复制该 Actor 时创建，Actor 被销毁或进入 Dormancy 后关闭。

`UChannel` 基类（`Channel.h:62`）本身是 `UObject` 的子类，`UPROPERTY() TObjectPtr<UNetConnection> Connection` 即其 Outer 链的体现，GC 可见性通过 `UPROPERTY` 保证（符合 D 模块 GC 可见性规则）。

---

## 练习 J1：Dedicated Server + Client 最小工程 + Actor Replication

### 目标

搭建可在 PIE 多人模式下运行的最小 replicated Actor，亲眼观察服务端 SpawnActor 如何在客户端自动出现，以及服务端移动如何同步到客户端，同时理解三种 Role 在不同端的取值。

**阶梯**：use

### 前置理解

- 模块 K 中 `AActor` 的生命周期钩子（`BeginPlay` / `Tick` / `EndPlay`）。
- `SpawnActor<T>` 在 `UWorld` 上的调用方式。
- `UWorld::GetNetMode()` 返回 `ENetMode`（`NM_DedicatedServer` / `NM_ListenServer` / `NM_Client` / `NM_Standalone`）。

### PIE 运行形态（必读，不得跳过）

本模块所有练习必须在以下 PIE 配置下运行：

1. 在 Unreal Editor 顶部菜单打开 **Edit → Editor Preferences → Play**，或直接点击 Play 按钮旁的下拉箭头选择 **Advanced Settings**。
2. 设置 **Number of Players = 2**。
3. 设置 **Net Mode = Play As Dedicated Server**（推荐；也可选 Listen Server，差异见进阶任务）。
4. 点击 Play。Editor 将打开两个视口：一个 Dedicated Server 窗口（无渲染画面），一个 Client 视口。
5. 在 Output Log 面板的下拉菜单中可切换查看 "Server" 或 "Client" 的日志，两侧日志分开显示。

若选择 **Play As Listen Server**，第一个视口既是服务端也是本地客户端（`NM_ListenServer`），第二个视口才是纯客户端。两种模式的 Actor Role 分配逻辑相同，但 Listen Server 侧不存在纯服务端的无渲染窗口。

### 必做任务

1. 新建一个 UELearn Module（参考模块 A 的 `.Build.cs` 范式），命名为 `J1_ReplicatedActor`，在 `PrivateDependencyModuleNames` 中添加 `"Core"`, `"CoreUObject"`, `"Engine"`。

2. 新建 `AJ1ReplicatedCube` 继承 `AActor`。在构造器里：
   ```cpp
   AJ1ReplicatedCube::AJ1ReplicatedCube()
   {
       PrimaryActorTick.bCanEverTick = true;
       bReplicates = true;   // 构造阶段设置，等价于 SetReplicates(true) 在初始化后调用
   }
   ```
   注意：`bReplicates = true` 在构造器里是正确写法；`SetReplicates(true)` 应在 `BeginPlay` 或更晚调用（`Actor.h:722` 的注释提示该函数处理网络对象列表注册，须在 World 完全初始化后）。

3. 在 `BeginPlay` 中打印当前 Role：
   ```cpp
   void AJ1ReplicatedCube::BeginPlay()
   {
       Super::BeginPlay();
       UE_LOG(LogTemp, Warning, TEXT("[J1] Actor=%s Role=%d RemoteRole=%d HasAuth=%d"),
           *GetName(),
           (int32)GetLocalRole(),
           (int32)GetRemoteRole(),
           (int32)HasAuthority());
   }
   ```

4. 在 `DefaultGameMode` 的 `BeginPlay` 或任意 Cheat/ConsoleCommand 里，仅在服务端调用 `SpawnActor<AJ1ReplicatedCube>`。观察：服务端日志和客户端日志里该 Actor 各打印什么 Role 值。

5. 在服务端 `Tick` 中每帧调用 `SetActorLocation` 移动该 Actor（沿 X 轴 +1 cm/frame）。在客户端 `Tick` 中打印 `GetActorLocation().X`。观察客户端位置是否跟随服务端变化——这依赖 `FRepMovement`（`Actor.h:793`，`UPROPERTY(ReplicatedUsing=OnRep_ReplicatedMovement)`）默认已注册 replication。

6. 验证 `FRepMovement` 的默认 replication：在 `Actor.cpp:2014` 中可看到 `DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(AActor, ReplicatedMovement, IsReplicatingMovement())`，说明运动同步是内置的，不需要学员手写 `DOREPLIFETIME`。

### 进阶任务

- 切换 `Net Mode = Play As Listen Server`，再观察 Role 打印值。Listen Server 的本地玩家控制的 Pawn 在服务端侧是 `ROLE_Authority`，在客户端侧是 `ROLE_AutonomousProxy`。
- 在 `Tick` 里用 `if (HasAuthority())` 守卫只在服务端调用 `SetActorLocation`，否则直接 return——这是 replication 代码的标准模式。
- 将 Actor 的 `NetUpdateFrequency` 调低（如 `NetUpdateFrequency = 2.0f`），观察客户端位置更新变卡顿，理解 NetDriver 的更新率控制。
- 观察 `ROLE_AutonomousProxy`：在 PIE 中让一个 `APlayerController` 持有 Pawn，服务端上该 Pawn 的 `Role = ROLE_Authority`，控制该 Pawn 的客户端上 `Role = ROLE_AutonomousProxy`，其余客户端上 `Role = ROLE_SimulatedProxy`。

### 验收点

- PIE 启动时，Output Log 中可以区分 Server / Client 两侧的 Role 日志，服务端打印 `Role=3`（`ROLE_Authority=3`），客户端打印 `Role=2`（`ROLE_SimulatedProxy=2`）。
- 服务端 SpawnActor 后，客户端无需任何代码即可看到该 Actor 出现（Actor Channel 的自动创建）。
- 服务端 SetActorLocation 后，客户端位置在 `1/NetUpdateFrequency` 秒内更新。
- 你能解释为什么不需要手写 `DOREPLIFETIME` 就能看到位置同步（答：`ReplicatedMovement` 是 `AActor` 基类已注册的 replicated property）。

### 常见坑 / 观察点 / 提示

- **坑：在客户端调用 SetActorLocation 后立刻被服务端覆盖**。客户端对 `ROLE_SimulatedProxy` 的 Actor 调用 `SetActorLocation` 会在下次 replication tick 被服务端的值覆盖，因为 Client 没有 Authority。用 `HasAuthority()` 守卫。
- **坑：PIE 只启动一个窗口**。检查 Advanced Settings → Number of Players 是否确实设置为 2。如果忘了设，所有 NetMode 相关代码都不会被触发。
- **观察**：`GetNetMode()` 在 Standalone 模式（单人）下返回 `NM_Standalone`，此时 `bReplicates` 虽为 true 但 NetDriver 不存在，任何 RPC 调用都会静默失败。开发阶段务必在 PIE 多人模式下验证网络功能。
- `bNetLoadOnClient`（`Actor.h:449`）控制该 Actor 是否在客户端加载地图时随关卡一起生成（与 Runtime spawn 不同），默认为 true。

### 复盘问题（四固定问题）

- **Q1：执行真正开始时刻？** 服务端 `SpawnActor` 触发 Actor 的构造和 `BeginPlay`（GameThread）；NetDriver 在每帧 `TickFlush`（GameThread）中调用 `ServerReplicateActors`，将 dirty properties 通过 Connection 发送到客户端；客户端在 `TickDispatch`（GameThread，但包解析可能在 NetThread）收到数据包后更新 Actor 状态，调用 `OnRep_ReplicatedMovement`（GameThread）。
- **Q2：生命周期拥有者？** `UNetDriver` 由 `UWorld` 持有（UPROPERTY，GC 可见）；`UNetConnection` 由 NetDriver 的 `ServerConnections` 数组持有（UPROPERTY）；`UActorChannel` 由 `UNetConnection` 的 Channels 数组持有（UPROPERTY，`Channel.h:68`，Connection 字段是 UPROPERTY TObjectPtr）；replicated Actor 由 `UWorld`/`ULevel` 持有（标准 Actor 生命周期，K 模块）。
- **Q3：涉及哪些 Named Thread？** 主要是 `GameThread`（`TickDispatch` / `TickFlush` / RPC 执行 / `OnRep_*` 回调）；包的底层收发可能在 `NetThread`（FReceiveThreadRunnable）或 TaskGraph worker pool；渲染无关，无 RenderThread 参与。
- **Q4：涉及 UObject 时怎么对 GC 可见？** `UActorChannel` 的 `Connection` 字段是 `UPROPERTY() TObjectPtr<UNetConnection>`（`Channel.h:68`）；replicated Actor 通过 `ULevel::Actors` 数组（UPROPERTY）保持 GC 可见；NetDriver 通过 `UWorld` 的 UPROPERTY 成员保持 GC 可见。整条 World→NetDriver→Connection→Channel→Actor 链路均通过 UPROPERTY 实现 GC 可见。

### 反射可见性图

```
AJ1ReplicatedCube
  ├── bReplicates            (非 UPROPERTY，C++ 位字段)
  ├── Role                   (TEnumAsByte<ENetRole>，Actor.h:827，内部字段)
  ├── RemoteRole             (TEnumAsByte<ENetRole>，Actor.h:711，UPROPERTY)
  └── ReplicatedMovement     ← UPROPERTY(ReplicatedUsing=OnRep_ReplicatedMovement)
                                对 GC 可见 ✓，对 replication 可见 ✓，对 Editor 可见 ✓
```

### 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h`：`bReplicates`（553行），`SetReplicates`（722行），`HasAuthority()`（1941/4964行），`Role`/`RemoteRole`（711/827行），`ReplicatedMovement`（793行）
- `Engine/Source/Runtime/Engine/Private/Actor.cpp`：`GetLifetimeReplicatedProps`（2014行，`DOREPLIFETIME_ACTIVE_OVERRIDE_FAST`）、`SetReplicates` 实现（4528行）
- `Engine/Source/Runtime/Engine/Classes/Engine/NetDriver.h`：三层结构注释（38-56行）
- `Engine/Source/Runtime/Engine/Classes/Engine/Channel.h`：`UChannel` 定义（62行），`Connection` UPROPERTY（68行）

---

## 练习 J2：Replicated UPROPERTY + RepNotify + RPC 三向

### 目标

手写完整的 replicated property（含 `DOREPLIFETIME`），实现 RepNotify 回调，并各写一个 Server / Client / NetMulticast RPC，深刻理解三向 RPC 的触发条件与执行端，以及 `_Implementation` 后缀的代码生成机制。

**阶梯**：use + inspect

### 前置理解

- J1 完成，理解 PIE 多人模式和 Role 三态。
- 模块 D 的 `UPROPERTY` 宏与 UHT 代码生成。
- `UFUNCTION` 与 UHT 的关系（`.generated.h` 中的 RPC 声明）。

### 必做任务

1. 新建 `AJ2RPCDemoActor`，继承 `AActor`，`bReplicates = true`。

2. 声明一个基础 replicated property：
   ```cpp
   UPROPERTY(Replicated)
   int32 ServerCounter = 0;
   ```
   在 `GetLifetimeReplicatedProps` 中注册：
   ```cpp
   void AJ2RPCDemoActor::GetLifetimeReplicatedProps(
       TArray<FLifetimeProperty>& OutLifetimeProps) const
   {
       Super::GetLifetimeReplicatedProps(OutLifetimeProps);
       DOREPLIFETIME(AJ2RPCDemoActor, ServerCounter);
   }
   ```
   在服务端 `Tick` 每秒递增 `ServerCounter`，在客户端 `Tick` 打印其值，观察同步。

3. 声明一个带 RepNotify 的 property：
   ```cpp
   UPROPERTY(ReplicatedUsing=OnRep_Health)
   float Health = 100.f;

   UFUNCTION()
   void OnRep_Health();
   ```
   `OnRep_Health` 在客户端收到新值后由引擎自动调用。在函数体中打印日志，观察它是否在服务端也被调用（答：不会；`OnRep_*` 只在接收端调用）。在 `GetLifetimeReplicatedProps` 中用 `DOREPLIFETIME(AJ2RPCDemoActor, Health)` 注册。

4. 实现 Server RPC：
   ```cpp
   // .h 声明
   UFUNCTION(Server, Reliable)
   void ServerRequestDamage(float Amount);

   // .cpp 实现（注意 _Implementation 后缀）
   void AJ2RPCDemoActor::ServerRequestDamage_Implementation(float Amount)
   {
       // 此处在服务端 GameThread 执行
       check(HasAuthority());
       Health = FMath::Max(0.f, Health - Amount);
   }
   ```
   在客户端 `BeginPlay` 里（`!HasAuthority()` 侧）调用 `ServerRequestDamage(10.f)`。

5. 实现 Client RPC（通常在服务端调用，发往特定客户端）：
   ```cpp
   UFUNCTION(Client, Reliable)
   void ClientNotifyDead();

   void AJ2RPCDemoActor::ClientNotifyDead_Implementation()
   {
       // 此处在拥有该 Actor 的客户端 GameThread 执行
       UE_LOG(LogTemp, Warning, TEXT("[J2] Client received: you are dead"));
   }
   ```
   在服务端侧 `Health` 降至 0 时调用 `ClientNotifyDead()`。

6. 实现 NetMulticast RPC：
   ```cpp
   UFUNCTION(NetMulticast, Reliable)
   void MulticastPlayEffect();

   void AJ2RPCDemoActor::MulticastPlayEffect_Implementation()
   {
       // 在服务端和所有客户端均执行
       UE_LOG(LogTemp, Warning, TEXT("[J2] Multicast on Role=%d"), (int32)GetLocalRole());
   }
   ```
   在服务端任意时机调用 `MulticastPlayEffect()`，观察日志在两侧出现。

7. 观察 `Reliable` vs `Unreliable`：将 `MulticastPlayEffect` 改为 `NetMulticast, Unreliable`，手动模拟丢包（在 Editor 中可用 `Net.PacketDropRate` console 变量模拟）。Reliable RPC 保证顺序送达，Unreliable RPC 允许丢包、带宽更低。

### 进阶任务

- 将 `ServerCounter` 改为 `DOREPLIFETIME_CONDITION(AJ2RPCDemoActor, ServerCounter, COND_OwnerOnly)`（`CoreNetTypes.h:23`），观察只有 Owning Client 能收到该值更新，其他客户端看到的仍是初始值 0。
- 用 `COND_InitialOnly` 注册一个 `SpawnTime` 字段，只在 Actor 首次出现时同步，后续不再更新。
- 尝试从 `ROLE_SimulatedProxy` 一侧调用 Server RPC，观察 UE 是否静默执行（UE 有权限检查，在某些配置下会发出警告或直接不执行）。
- 在头文件中找到 UHT 为 RPC 生成的内容：在 IDE 中打开对应的 `.generated.h`，找到 `ServerRequestDamage_Validate` 声明（若有 `WithValidation`）和 RPC thunk。理解 `_Implementation` 后缀是 UHT 的约定，真正的调用分发由 thunk 函数完成。

### 验收点

- `ServerCounter` 在客户端每秒递增，不需要任何客户端代码驱动。
- `OnRep_Health` 在客户端收到新值后被调用，打印日志，且服务端不调用该函数。
- Server RPC 在 `!HasAuthority()` 侧调用后，在服务端 `_Implementation` 里可用 `check(HasAuthority())` 验证。
- Client RPC 只在目标客户端执行（Owner），不在服务端执行（用日志区分）。
- Multicast RPC 在服务端和两个客户端均打印日志。
- 你能在 `.generated.h` 中找到 RPC 相关的 thunk 函数声明。

### 常见坑 / 观察点 / 提示

- **坑：忘写 `Super::GetLifetimeReplicatedProps`**。如果漏掉 `Super` 调用，基类的 `ReplicatedMovement` 等字段不会被注册，导致位置同步失效。
- **坑：`DOREPLIFETIME` 漏写导致字段不同步**。声明了 `UPROPERTY(Replicated)` 但忘写 `DOREPLIFETIME`，字段永远不会同步，且 UE 在某些版本会在运行时打印警告。
- **坑：把 Client RPC 当 Multicast 用**。`Client` RPC 只发往拥有该 Actor 的 PlayerController 对应的客户端，不是广播。需要广播用 `NetMulticast`。
- **坑：在没有 Authority 的端调用 Client/Multicast RPC**。这些 RPC 必须从有 Authority 的端（服务端）发出；客户端直接调用会静默失效。
- **`_Implementation` 后缀**：UHT 为每个 RPC 生成一个同名的 thunk 函数（无后缀），当你调用 `ServerRequestDamage(10.f)` 时，实际调用的是 thunk，它负责序列化参数并通过 Connection 发送；在接收端，thunk 的反面负责反序列化并调用 `_Implementation`。你的业务逻辑永远写在 `_Implementation` 里。
- **RepNotify 与 COND_** 的组合：`ReplicatedUsing=` 要求被注册的条件必须包含该客户端，否则 OnRep 不会被调用。

### 复盘问题（四固定问题）

- **Q1：执行真正开始时刻？** `DOREPLIFETIME` 的注册在 `GetLifetimeReplicatedProps` 里，该函数在 CDO 初始化阶段和 Actor 复制初始化时被调用（GameThread）。Server RPC 的 `_Implementation` 在服务端 `TickDispatch` 处理接收包时，在 GameThread 上被调用。`OnRep_*` 在客户端 `TickDispatch` 应用 replicated property 新值后，在 GameThread 上被调用。
- **Q2：生命周期拥有者？** `FLifetimeProperty` 条目存在 `TArray<FLifetimeProperty>` 栈变量中，由 NetDriver 的 replication machinery 使用后丢弃；RPC 参数被序列化为网络包并通过 `UNetConnection` 发送，包的生命周期由 Connection 的发送缓冲区管理；`Health` 等 replicated property 与 Actor 共享生命周期（Actor 活着字段就活着）。
- **Q3：涉及哪些 Named Thread？** 全程 `GameThread`：`GetLifetimeReplicatedProps` 调用、`ServerReplicateActors` 驱动、`OnRep_*` 回调、RPC `_Implementation` 执行。底层包 I/O 可能涉及 NetThread，但对业务逻辑不可见。
- **Q4：涉及 UObject 时怎么对 GC 可见？** `Health` / `ServerCounter` 是 POD 字段（`float`/`int32`），GC 不追踪；若 replicated property 是 `UObject*` 类型，必须加 `UPROPERTY`（同时用于 GC 可见性和 replication 注册），两个目的由同一个宏实现。

### 对应官方参考

- `Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h`：`DOREPLIFETIME`（259行）、`DOREPLIFETIME_CONDITION`（277行）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/CoreNetTypes.h`：`COND_*` 枚举（21-38行）
- `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h`：`GetLifetimeReplicatedProps`（273行）
- `Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerController.h`：RPC 声明样例（2297行 `UFUNCTION(Client, Reliable)`，2369行 `UFUNCTION(Server, Reliable)`）

---

## 练习 J3：自定义 FFastArraySerializer / NetSerialize 特化

### 目标

理解为什么经典全量 replication 对大型动态数组效率低下，通过派生 `FFastArraySerializer` 实现增量同步，并理解 `NetSerialize` 中 `FArchive::IsSaving()` / `IsLoading()` 的网络语义（对应 E 模块的序列化知识）。

**阶梯**：inspect → implement-own

### 前置理解

- J2 完成，理解 `DOREPLIFETIME` 与 `UPROPERTY(Replicated)` 的关系。
- 模块 E 中 `FArchive` 的双向序列化惯例（`IsSaving` = 写入方向，`IsLoading` = 读取方向）。
- `USTRUCT` + `GENERATED_USTRUCT_BODY()` 的基本写法（D 模块）。

### 必做任务

1. **阅读 `FastArraySerializer.h`**：仔细阅读文件头部的 6 步使用说明（`FastArraySerializer.h:62-117`）。理解：
   - Step 1：元素结构继承 `FFastArraySerializerItem`
   - Step 2：包装结构继承 `FFastArraySerializer`
   - Step 3：包装结构内必须有名为 `Items` 的 `TArray`
   - Step 4：实现 `NetDeltaSerialize`，转调 `FFastArraySerializer::FastArrayDeltaSerialize`
   - Step 5：为包装结构特化 `TStructOpsTypeTraits<>` 并设置 `WithNetDeltaSerializer = true`

2. 实现一个简单的 inventory item 数组：
   ```cpp
   USTRUCT()
   struct FJ3InventoryItem : public FFastArraySerializerItem
   {
       GENERATED_USTRUCT_BODY()

       UPROPERTY()
       int32 ItemID = 0;

       UPROPERTY()
       int32 Quantity = 0;

       void PostReplicatedAdd(const struct FJ3InventoryList& InArraySerializer);
       void PostReplicatedChange(const struct FJ3InventoryList& InArraySerializer);
       void PreReplicatedRemove(const struct FJ3InventoryList& InArraySerializer);
   };

   USTRUCT()
   struct FJ3InventoryList : public FFastArraySerializer
   {
       GENERATED_USTRUCT_BODY()

       UPROPERTY()
       TArray<FJ3InventoryItem> Items;

       bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
       {
           return FFastArraySerializer::FastArrayDeltaSerialize<
               FJ3InventoryItem, FJ3InventoryList>(Items, DeltaParms, *this);
       }
   };

   template<>
   struct TStructOpsTypeTraits<FJ3InventoryList>
       : public TStructOpsTypeTraitsBase2<FJ3InventoryList>
   {
       enum { WithNetDeltaSerializer = true };
   };
   ```

3. 在 `AJ3InventoryActor` 中声明该 property 并注册：
   ```cpp
   UPROPERTY(Replicated)
   FJ3InventoryList Inventory;
   // GetLifetimeReplicatedProps 里：
   DOREPLIFETIME(AJ3InventoryActor, Inventory);
   ```

4. 在服务端每隔 2 秒添加一个新 item：
   ```cpp
   FJ3InventoryItem& NewItem = Inventory.Items.AddDefaulted_GetRef();
   NewItem.ItemID = FMath::RandRange(1, 100);
   NewItem.Quantity = 1;
   Inventory.MarkItemDirty(NewItem);   // 必须调用！告知 FastArray 该元素 dirty
   ```
   在客户端的 `PostReplicatedAdd` 回调里打印日志，观察每次只有新增元素触发回调，而不是整个数组。

5. 在服务端修改一个已有 item 的 `Quantity`，调用 `Inventory.MarkItemDirty(Inventory.Items[0])`，观察客户端 `PostReplicatedChange` 被调用。

6. **阅读增量序列化原理**（`FastArraySerializer.h:140-150`）：理解 `UActorChannel::Recent` 缓冲区是经典 replication 的全量比较基础；FastArray 用每元素的 `ReplicationID` + `ReplicationKey` 代替全量比较，只序列化 dirty 元素。

### 进阶任务

- **自定义 `NetSerialize`**：为一个简单的 `FJ3CompressedVector` struct 实现 `NetSerialize`：
   ```cpp
   USTRUCT()
   struct FJ3CompressedVector
   {
       GENERATED_USTRUCT_BODY()

       UPROPERTY()
       float X = 0.f;
       UPROPERTY()
       float Y = 0.f;

       bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
       {
           if (Ar.IsSaving())
           {
               // 发送端：量化压缩到 10 位整数
               int32 QX = FMath::RoundToInt(X * 10.f);
               int32 QY = FMath::RoundToInt(Y * 10.f);
               Ar << QX << QY;
           }
           else  // Ar.IsLoading()
           {
               // 接收端：反量化
               int32 QX, QY;
               Ar << QX << QY;
               X = QX / 10.f;
               Y = QY / 10.f;
           }
           bOutSuccess = true;
           return true;
       }
   };
   template<>
   struct TStructOpsTypeTraits<FJ3CompressedVector>
       : public TStructOpsTypeTraitsBase2<FJ3CompressedVector>
   {
       enum { WithNetSerializer = true };
   };
   ```
   对照 E 模块的 `FArchive::operator<<`：在序列化路径下，`IsSaving() = true` 是发送端打包，`IsLoading() = true` 是接收端解包，同一份代码跑两个方向。

- 观察不调用 `MarkItemDirty` 的后果：手动修改 `Items[0].Quantity`，但不调用 `MarkItemDirty`——客户端不会收到更新，理解 FastArray 的 dirty tracking 依赖手动标记。

### 验收点

- 服务端每次添加 item 后，客户端 `PostReplicatedAdd` 打印对应 ItemID，且确认只有新增元素触发，非整个数组重传。
- 服务端修改 `Quantity` 并调用 `MarkItemDirty` 后，客户端 `PostReplicatedChange` 被调用。
- `NetSerialize` 自定义版本：客户端收到的 `FJ3CompressedVector` 值与服务端原始值误差在 0.1 以内（量化精度）。
- 你能解释 `FArchive::IsSaving()` 在网络路径下对应哪端，以及与 E 模块的资产序列化用法有何相同之处。

### 常见坑 / 观察点 / 提示

- **坑：忘调 `MarkItemDirty` / `MarkArrayDirty`**。FastArray 的核心假设是"游戏代码主动标记 dirty"，引擎不会自动检测元素内部的字段变化。移除元素后必须调用 `MarkArrayDirty`，修改元素后必须调用 `MarkItemDirty`。
- **坑：Items 数组字段名不叫 `Items`**。FastArray 的模板机制要求包装结构的 TArray 字段名必须是 `Items`（或通过特化 `TFastArraySerializer::ItemsArrayType` 自定义，但默认要求 `Items`）。
- **观察**：FastArray 不保证客户端与服务端的数组元素顺序完全一致——元素可能被移位以填补删除空洞（`FastArraySerializer.h:54`："the *order* of the list is not guaranteed to be identical between client and server in all cases"）。

### 复盘问题（四固定问题）

- **Q1：执行真正开始时刻？** `NetDeltaSerialize` 在 `UActorChannel::ReplicateActor`（GameThread，每帧 `TickFlush`）中被调用；`PostReplicatedAdd` / `PostReplicatedChange` 在客户端 `TickDispatch` 应用 delta 时调用（GameThread）。
- **Q2：生命周期拥有者？** `FJ3InventoryList` 作为 Actor 的 UPROPERTY 成员，生命周期与 Actor 同步；`FFastArraySerializer` 内部的 `ItemMap`（dirty tracking 元数据）也随 struct 生命周期存活；临时的 `FNetDeltaSerializeInfo` 是栈变量，`ReplicateActor` 调用结束即销毁。
- **Q3：涉及哪些 Named Thread？** 全程 GameThread。`FastArrayDeltaSerialize` 是 CPU 密集型序列化逻辑，但在 UE 经典 replication 路径下仍在 GameThread 上执行（不像 Iris 路径可以并行）。
- **Q4：涉及 UObject 时怎么对 GC 可见？** `FJ3InventoryList` 是非 UObject struct（USTRUCT），GC 通过 UPROPERTY 追踪包含它的 Actor 字段；struct 内的 `TArray<FJ3InventoryItem>` 里若有 `UPROPERTY` 字段指向 UObject，需在元素 struct 中加 UPROPERTY；本例中 ItemID/Quantity 是 int32，无 GC 追踪需求。

### 对应官方参考

- `Engine/Source/Runtime/Net/Core/Classes/Net/Serialization/FastArraySerializer.h`：完整 6 步使用说明（62-117行）、增量序列化原理注释（140-150行）、`PostReplicatedAdd`/`PostReplicatedChange`/`PreReplicatedRemove` 回调约定（83-88行）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/CoreNet.h`：`FFieldNetCache` / `FClassNetCache`（72-100行）

---

## 练习 J4：网络预测与回滚最小示例（Inspect）

### 目标

阅读 `UCharacterMovementComponent` 中客户端预测（client prediction）的骨架，理解 `FSavedMove_Character`、Server RPC（ServerMovePacked）、Client RPC（ClientAdjustPosition）三者协作的时序，能画出一张 client input tick 与 server ack 的时序图。

**阶梯**：inspect（不要求手写完整 CMC，只需读懂时序）

### 前置理解

- J1/J2 完成，理解 Server RPC / Client RPC 机制。
- 模块 K 中 `APlayerController` → `APawn` → `UPawnMovementComponent` 的层级关系。
- 基本的刚体运动物理概念（位置、速度、加速度）。

### 网络预测的核心问题

没有预测时，客户端每次按键需要：
1. 将输入发送到服务端（~RTT/2 延迟）
2. 服务端模拟并将结果发回（~RTT/2 延迟）
3. 客户端才能看到自己的角色移动

总延迟 = 完整 RTT（100-200ms）。玩家感知到明显的输入延迟。

**解决方案**：客户端预测（Client-side Prediction） + 服务端校正（Server Reconciliation）：
- 客户端在本地立即应用输入（零延迟感知）
- 同时将输入发往服务端
- 服务端模拟后若发现偏差超过阈值，下发校正指令
- 客户端收到校正后回滚到服务端位置，再重播未确认的历史输入

### 必做任务（阅读任务）

1. **阅读 `CharacterMovementComponent.h` 的预测注释**（2350-2371行）：
   ```
   to the server by calling the replicated function ServerMove() - passing the movement parameters, 
   the client's timestamp.
   ServerMove() is executed on the server. It decodes the movement parameters and causes the 
   appropriate movement to occur. If a position error is significant enough, the server calls 
   ClientAdjustPosition(), a replicated function.
   ClientAdjustPosition() is executed on the client. The client sets its position to the 
   servers version of position...
   ```
   这是 UE 官方对 prediction 时序的精炼描述。

2. **理解 `FSavedMove_Character`**（`CharacterMovementReplication.h:17`，`CharacterMovementComponent.h:116`）：客户端每帧的输入被保存在一个 `FSavedMovePtr`（`TSharedPtr<FSavedMove_Character>`）对象中，包含：时间戳、输入向量（加速度方向）、压缩标志（jump/crouch/sprint）、预测后的位置/速度。这组历史输入队列是 reconciliation 的基础。

3. **理解调用链**（`CharacterMovementComponent.h:2373-2379`）：
   - 客户端每 tick 调用 `CallServerMovePacked` → `ServerMovePacked_ClientSend` → Server RPC `ServerMovePacked`（发往服务端）
   - 服务端执行 `ServerMoveHandleClientError`（`CharacterMovementComponent.h:2387`）比较服务端位置与客户端上报位置
   - 若误差超过 `MAXPOSITIONERRORSQUARED`，服务端调用 Client RPC `ClientAdjustPosition`（`CharacterMovementReplication.h:252`，`ClientAdjustPosition replication (event called at end of frame by server)`）
   - 客户端收到 `ClientAdjustPosition` 后，将位置重置为服务端值，然后重播 `SavedMoves` 队列中所有未被 ack 的历史输入

4. **绘制时序图**（必做，手绘或 ASCII）：

   ```
   时间轴 →
   
   Client  | T1:输入↑ | 本地预测=P1 | T2:输入↑ | 本地预测=P2 | ... 收到Ack(T1)→确认P1 | 收到Adjust→回滚+重播 |
           |  ServerMove(T1,input)→→→→→→→→→→→↓             |                        |
   Server  |                  收到T1 | 服务端模拟=S1 | 比较P1≈S1? | 是→ClientAckGoodMove(T1)→→→→→→↑
           |                  若|P1-S1|>阈值 → ClientAdjustPosition(S1)→→→→→→→→→→→→→→→→→→→↑
   ```

   关键理解：
   - 客户端 SavedMoves 队列在等待 ack；ack 到达后删除该条目
   - 若收到 `ClientAdjustPosition`，则从 SavedMoves 里取出所有 T>Tadj 的历史输入，逐一重播，得到最终客户端位置
   - `NetworkMinTimeBetweenClientAckGoodMoves`（`CharacterMovementComponent.h:850`）控制 ack 的发送频率，减少带宽

5. 在 PIE 模式下，打开 console（`~` 键），输入 `p.NetShowCorrections 1`，在 debug 视图中观察客户端被 server 校正时的橡皮筋效果（红色箭头表示被纠正的位移）。

### 进阶任务

- 阅读 `FSavedMove_Character::ClientFillNetworkMoveData`（`CharacterMovementReplication.h:128`），理解客户端如何将当前帧的输入状态打包到 Server RPC 参数中。
- 思考"为什么 `FSavedMove_Character` 用 `TSharedPtr` 管理而不是 UObject"：它是每帧临时分配、引用计数管理的 C++ 对象，不需要 GC；反射系统对其不可见——这是 D 模块"非 UObject 用 TSharedPtr"规则的典型应用（`CharacterMovementComponent.h:117`，`typedef TSharedPtr<class FSavedMove_Character> FSavedMovePtr`）。
- 研究时间差异检测：`CharacterMovementComponent.h:2455-2475` 中的注释描述了 `CurrentTimeDiscrepancy`（有界累积时间差）和 `LifetimeRawTimeDiscrepancy`（无界累积），这是防作弊（客户端报告时间与服务端时间不符）的机制。
- 设计自己的"最小 prediction 结构"（不要求实现）：如果你要为一个飞船Actor实现 prediction，需要保存哪些状态（位置/速度/朝向/推进力），如何在 Server RPC 中传递，如何在 `ClientAdjustPosition` 等价函数中执行回滚？写下伪代码设计。

### 验收点

- 你能不看源码，用自己的话复述"客户端预测 + 服务端校正"的完整时序（5步以内）。
- 时序图中正确标出 Server RPC 和 Client RPC 的方向（Client→Server 与 Server→Client）。
- 你能解释为什么 `FSavedMovePtr` 用 `TSharedPtr` 而不是裸指针或 UPROPERTY UObject*。
- 你能在 PIE 中触发并观察到 `p.NetShowCorrections 1` 的校正可视化。

### 常见坑 / 观察点 / 提示

- **不要尝试手写完整 CMC**：`UCharacterMovementComponent` 有数千行处理各种边界情况（台阶检测、斜面滑行、网络时间同步、移动模式切换），这是 UE 多年积累的成果。本题只需读懂主干时序。
- **观察**：prediction 的代价是客户端与服务端各独立模拟一次移动，消耗双份 CPU；reconciliation 在校正时需要重播 N 帧历史输入，校正幅度越大、延迟越高，重播代价越高。这是"零延迟感知"的代价。
- **`AutonomousProxy` 是 prediction 的角色基础**：只有 `ROLE_AutonomousProxy`（本地玩家控制的 Pawn）才做 prediction；`ROLE_SimulatedProxy`（其他玩家的 Pawn）只做 interpolation（在收到的两个服务端快照之间插值），不做 prediction。

### 复盘问题（四固定问题）

- **Q1：执行真正开始时刻？** 客户端预测在 `UCharacterMovementComponent::TickComponent`（GameThread，每帧）开始；Server RPC 的 `_Implementation` 在服务端下一次 `TickDispatch`（GameThread）处理接收包时执行；`ClientAdjustPosition_Implementation` 在客户端下一次 `TickDispatch`（GameThread）执行，重播历史输入在该帧内同步完成。
- **Q2：生命周期拥有者？** `FSavedMovePtr`（`TSharedPtr<FSavedMove_Character>`）由 `FNetworkPredictionData_Client_Character` 拥有，该对象由 `UCharacterMovementComponent` 创建并持有；当 Server ack 到达时对应条目从队列中删除，引用计数归零，自动析构——完全是非 UObject 的 TSharedPtr 生命周期管理（模块 C 的 use 级阶梯）。
- **Q3：涉及哪些 Named Thread？** 全程 GameThread（CMC tick、RPC 发送/接收、历史输入重播）；底层 UDP 包的收发在 NetThread；无 RenderThread 参与。
- **Q4：涉及 UObject 时怎么对 GC 可见？** `UCharacterMovementComponent` 本身是 UActorComponent（UObject 子类），通过 Actor 的 `UPROPERTY() TObjectPtr<UCharacterMovementComponent>` 对 GC 可见；`FSavedMove_Character` 不是 UObject，通过 `TSharedPtr` 管理，GC 不追踪它。

### 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h`：prediction 注释（2350-2371行）、`CallServerMovePacked`（2379行）、`ServerMoveHandleClientError`（2387行）、`NetworkMinTimeBetweenClientAckGoodMoves`（850行）
- `Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementReplication.h`：`FSavedMove_Character` 前向声明（17行）、`ClientFillNetworkMoveData`（128行）、`ClientAdjustPosition` 注释（252行）

---

## §Iris 展望：经典 Replication 路径之外

UE 5.5 及之后版本引入了 **Iris**（`Runtime/Net/Iris/` 及 `Runtime/Experimental/Iris/`），作为经典 `UNetDriver` replication 路径的实验性替代。入口头文件位于 `Engine/Source/Runtime/Net/Iris/Public/Iris/ReplicationSystem/ReplicationSystem.h`（在本机为 `Engine/Source/Runtime/Net/Iris/Public/Iris/ReplicationSystem/ReplicationSystem.h`，`UReplicationSystem` 类定义于第 69 行）。

**经典路径 vs Iris 路径的本质差异**：

经典路径（本模块所有练习的基础）：
- 以 Actor 为单位、基于反射元数据的 per-property dirty tracking
- `UActorChannel::Recent` 缓冲区存储上次发送值，逐字段对比
- Actor 串行处理：`ServerReplicateActors` 遍历所有相关 Actor，逐一创建 `FOutBunch` 并序列化

Iris 路径（`UReplicationSystem`，`ReplicationSystem.h:69`）：
- 以 `FNetRefHandle`（`ReplicationSystem.h:73`）而非 `UObject*` 标识网络对象，解耦对象标识与 GC 图
- 批处理（Batching）：多个 Actor 的 replication state 在一个 pass 里处理，CPU 缓存更友好
- 更细粒度的 fragmentation：replication state 被切分为独立的 `FReplicationStateDescriptor`，允许部分更新
- `FNetObjectFilter` / `FNetObjectPrioritizer`（`ReplicationSystem.h:27-29`）是 relevancy culling 和优先级调度的可插拔接口，经典路径里这些是硬编码在 `AActor::IsNetRelevantFor` 和 `AActor::GetNetPriority` 里的

**本课程定位**：Iris 目前（UE 5.7 release）仍处于 `Runtime/Experimental/` 路径下，API 有漂移风险，不纳入必做练习。了解其存在对阅读引擎源码有帮助：当你在 `Actor.h:3479` 看到注释"Called if using iris replication after an actor has started to replicate with iris replication"时，你知道这是 Iris 路径专用的回调，与经典路径的 `BeginPlay` 钩子并列存在。

如果你想探索 Iris，起点是 `UReplicationSystem::FReplicationSystemParams`（`ReplicationSystem.h:79`）和 `UObjectReplicationBridge`（`ReplicationSystem.h:31`）。

---

## 做完模块 J 之后，你现在应该能说清楚什么

至少把下面几句话说顺：

- **为什么 replication 完全依赖 D 模块的反射**：`UPROPERTY(Replicated)` 让 UHT 产生反射元数据，`GetLifetimeReplicatedProps` 里的 `DOREPLIFETIME` 利用这套元数据注册需要同步的字段；NetDriver 在 `ServerReplicateActors` 中通过反射系统枚举所有 registered properties，读取当前值并与 Recent 缓冲区比较。没有 D 模块的 FProperty 链，整个 replication 无从运作。

- **为什么 FArchive 的双向语义在网络路径中依然成立**：`NetSerialize` 的签名 `bool NetSerialize(FArchive& Ar, ...)` 与 E 模块的资产序列化共享相同的 `FArchive` 基类；`Ar.IsSaving()` 在发送端为真，`Ar.IsLoading()` 在接收端为真，同一份代码跑两个方向——这是 E 模块序列化设计在网络层的直接延伸。

- **为什么网络任务在 F 模块的线程模型里有明确位置**：`TickDispatch`（包接收）和 `TickFlush`（包发送/ServerReplicateActors）都发生在 GameThread；底层 I/O 可能在 NetThread；RPC 的 `_Implementation` 在 GameThread 执行——这与 F 模块"GameThread 是游戏逻辑的唯一执行者"的原则一致。

- **为什么 K 模块的 Actor Role 是理解 RPC 触发条件的前提**：`ROLE_Authority` 决定谁能执行 Server RPC 的 `_Implementation`，`ROLE_AutonomousProxy` 决定谁能发起 Server RPC，`ROLE_SimulatedProxy` 决定谁只接收 replication 更新。没有 K 模块建立的 Role 概念，RPC 的"从哪里调用、在哪里执行"无法被正确理解。

- **FastArray 的核心价值**：不是"大数组能用"，而是"只传变化的元素"。经典 replication 的粒度是整个字段，FastArray 的粒度是单个元素，依赖游戏代码的显式 `MarkItemDirty` 调用和 per-element 版本号实现。

- **网络预测的两个关键不对称**：客户端 `AutonomousProxy` 做 prediction（本地先行，再校正），客户端 `SimulatedProxy` 做 interpolation（快照间插值）；服务端始终是 Authority，它的模拟结果是最终真相，客户端的 prediction 只是延迟补偿的近似。

---

## 本模块覆盖的 UE 源码清单

| 文件 | 关键内容 | 练习 |
|---|---|---|
| `Engine/Source/Runtime/Engine/Classes/Engine/NetDriver.h` | 三层结构注释（38-56行），Driver 类型定义 | J1 |
| `Engine/Source/Runtime/Engine/Classes/Engine/NetConnection.h` | `FActorChannelMap` typedef（64行），Connection 与 Channel 关系 | J1 |
| `Engine/Source/Runtime/Engine/Classes/Engine/Channel.h` | `UChannel` 定义（62行），`Connection` UPROPERTY（68行），`EChannelType`（25-35行） | J1 |
| `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h` | `bReplicates`（553/556行），`SetReplicates`（722行），`Role`/`RemoteRole`（711/827行），`HasAuthority()`（1941/4964行），`ReplicatedMovement`（793行），`GetLifetimeReplicatedProps`（273行） | J1/J2 |
| `Engine/Source/Runtime/Engine/Private/Actor.cpp` | `DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(AActor, ReplicatedMovement, ...)`（2014行），`SetReplicates` 实现（4528行） | J1/J2 |
| `Engine/Source/Runtime/Engine/Public/Net/UnrealNetwork.h` | `DOREPLIFETIME`（259行），`DOREPLIFETIME_CONDITION`（277行），`DOREPLIFETIME_WITH_PARAMS`（250行） | J2 |
| `Engine/Source/Runtime/CoreUObject/Public/UObject/CoreNetTypes.h` | `ELifetimeCondition` / `COND_*` 枚举（21-38行） | J2 |
| `Engine/Source/Runtime/CoreUObject/Public/UObject/CoreNet.h` | `FFieldNetCache`（72行），`FClassNetCache`（88行），`FNetworkGUID` 相关结构 | J2/J3 |
| `Engine/Source/Runtime/Engine/Classes/GameFramework/PlayerController.h` | Client RPC 示例（2297行），Server RPC 示例（2369行） | J2 |
| `Engine/Source/Runtime/Net/Core/Classes/Net/Serialization/FastArraySerializer.h` | 6 步使用说明（62-117行），增量序列化原理（140-150行），回调约定（83-88行） | J3 |
| `Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h` | Prediction 注释（2350-2371行），`FSavedMovePtr`（117行），`NetworkMinTimeBetweenClientAckGoodMoves`（850行） | J4 |
| `Engine/Source/Runtime/Engine/Classes/GameFramework/CharacterMovementReplication.h` | `ClientFillNetworkMoveData`（128行），`ClientAdjustPosition` 注释（252行） | J4 |
| `Engine/Source/Runtime/Net/Iris/Public/Iris/ReplicationSystem/ReplicationSystem.h` | `UReplicationSystem`（69行），`FNetRefHandle`（73行），Iris 批处理架构 | §Iris 展望 |
