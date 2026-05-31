# 06 模块 K：Actor / Component / World / Subsystem

## 模块目标

反向引用 `01-心智模型` 的 §4（GC ownership 与 C++ ownership 两套并存）与 §6（Subsystem 化，而非全局单例）。

模块 D 讲清了 UObject/反射/GC 的机制层：`NewObject`、`UPROPERTY`、mark-sweep 遍历。但如果你现在打开一份真实 UE 工程，映入眼帘的不是裸 `UObject`，而是 `AActor`、`UActorComponent`、`UWorld`、`UGameInstance`、`UWorldSubsystem`——这些都是 UObject 子类构成的"可运行对象生态"。

本模块的目标是从 UObject 机制层跨入这套生态，回答三个问题：

1. 一个 `AActor` 从 `UWorld::SpawnActor` 调用到 `Destroyed` 析构，经过哪些钩子，每个钩子的职责边界在哪里？
2. `UActorComponent` 和 `USceneComponent` 如何给 Actor 叠加能力，transform 层级如何建立，运行时动态添加组件走哪条路径？
3. `UWorld` / `ULevel` / `UGameInstance` 是什么关系，`SpawnActor` 内部如何把新 Actor 挂进 `ULevel::Actors`？"全局唯一对象"在 UE 里如何用 Subsystem 代替裸单例，三层 Subsystem 的生命周期锚点各是什么？

**阶梯定位**：use + inspect。练习 K1、K2、K4 侧重 use（写出代码并观察行为），练习 K3 侧重 inspect（追进源码画出链路）。

**四固定问题在本模块的特殊含义**：

- Q4（GC 可见性）在本模块转化为"谁被 World/Outer 链路 reachable"——Actor 的 Outer 是 ULevel，Component 的 Outer 是 Actor；链路断了 = GC 不可达。

---

## 模块完成标准

做完本模块，至少要能稳定说清楚：

- `PostInitializeComponents`、`BeginPlay`、`Tick`、`EndPlay`、`Destroyed` 五钩子分别在引擎生命周期的哪个时刻调用，构造函数 vs `BeginPlay` 能做的事有什么本质差异。
- `UActorComponent::SetupAttachment` 与 `AttachToComponent` 的区别（构造期 vs 运行期），`RegisterComponent` 做了什么，为什么运行时新增组件必须显式调用它。
- `UWorld` → `PersistentLevel` + `StreamedLevels` → `ULevel::Actors` 三层链路，`SpawnActor` 内部如何用 `NewObject<AActor>(LevelToSpawnIn, ...)` 把 Actor 的 Outer 设为 `ULevel`。
- Subsystem 是 UE 对"按生命周期层绑定的单例"的规范实现，三层（Engine / GameInstance / World）生命周期锚点不同，PIE 对各层的影响不同；裸全局单例在 PIE 场景下的问题是什么。

---

## GC 可见性在本模块的诠释：Outer 链路

在模块 D 里，GC 可见性的讨论集中在 `UPROPERTY` 标记与 `TObjectPtr`。本模块引入另一条等效路径：**Outer 链（Outer chain）**。

每个 `UObject` 在创建时都有一个 Outer——"我归属于谁"。UE 的 GC 遍历从根集出发，沿两种边扩散：

1. `UPROPERTY` 标记的指针成员（显式引用边）。
2. `GetOuter()` 链（隐式归属边）：GC 标记一个对象时，该对象下所有以它为 Outer 的对象也被认为是可达的。

在 Actor/Component/World 体系里，这条链路是：

```
UEngine（根集）
  └── UGameInstance（Outer = UEngine）
        └── UWorld（Outer = UGameInstance or UPackage）
              └── ULevel（PersistentLevel / StreamedLevels）
                    └── AActor（Outer = ULevel）
                          └── UActorComponent（Outer = AActor）
```

**关键推论**：

- Actor 的 Outer 是 `ULevel`。只要 Level 还在 World 里，Actor 就沿 Outer 链 reachable，GC 不会踢掉它。
- Component 的 Outer 是 Actor（通过 `CreateDefaultSubobject` 创建时 Outer = this Actor）。Actor 活着，Component 就活着。
- 如果你用 `NewObject<UActorComponent>(SomeOtherObject, ...)` 把 Component 的 Outer 设成了不在 World 链上的对象，那条 Outer 链断了，GC 可能回收该 Component，即使你还拿着裸指针——这是 Component 管理里最容易犯的 GC 坑。
- `UGameInstanceSubsystem` 的 Outer 是 `UGameInstance`；`UWorldSubsystem` 的 Outer 是 `UWorld`。只要宿主活着，Subsystem 就可达。

源码验证：`LevelActor.cpp` 第 671 行：

```cpp
// actually make the actor object
AActor* const Actor = NewObject<AActor>(LevelToSpawnIn, Class, NewActorName, ActorFlags, Template, ...);
check(Actor->GetLevel() == LevelToSpawnIn);
```

第一个参数 `LevelToSpawnIn` 就是新 Actor 的 Outer。`GetLevel()` 直接返回 `Cast<ULevel>(GetOuter())`，和 Outer 链完全对应。

---

## 练习 K1 — Hello AActor + 生命周期五钩子

### 目标

观察一个 `AActor` 从 Spawn 到销毁走过的完整钩子序列，理解每个钩子调用时机、World 状态和 Component 注册状态之间的关系。

### 前置理解

- 完成模块 D（`NewObject` 与 UObject 生命周期、`UPROPERTY`、GC 根集）。
- 知道 `UCLASS`/`GENERATED_BODY()` 宏的含义，知道 `UE_LOG` 的基本用法。
- 理解 PIE（Play In Editor）是 Editor 内的沙盒世界，PIE 开始时 `UWorld` 被创建，PIE 停止时 `UWorld` 被销毁。

### 必做任务

1. 在你的 UELearn 工程下新建一个练习 Module，名称建议 `K1_ActorLifecycle`。`.Build.cs` 只需 `PublicDependencyModuleNames.Add("Engine")`（以及 Core/CoreUObject）。

2. 派生 `AActor`，类名建议 `ALifecycleActor`。在头文件中 override 以下函数（注意加 `virtual ... override`）：
   - `PreInitializeComponents()`
   - `PostInitializeComponents()`
   - `BeginPlay()`
   - `Tick(float DeltaTime)`
   - `EndPlay(const EEndPlayReason::Type EndPlayReason)`
   - `Destroyed()`

3. 在每个函数实现里，用 `UE_LOG(LogTemp, Warning, TEXT("[K1] %s"), TEXT("<函数名>"))` 打印一条带函数名的日志。在 `BeginPlay` 里额外打印 `GetWorld() != nullptr`（证明此时 World 确实存在）。在构造函数里也打印一条日志，观察构造函数 vs `BeginPlay` 的顺序。

4. 在 Editor 里创建一个空关卡（或用已有关卡），把 `ALifecycleActor` 拖进场景，或在 World Settings 里把 GameMode 换成自定义 GameMode，在 `AMyGameMode::BeginPlay()` 里用 `GetWorld()->SpawnActor<ALifecycleActor>(...)` 动态生成一个实例。

5. PIE 流程：点击 Play（PIE 开始）→ 等 2-3 秒（观察 Tick 日志）→ 点击 Stop（PIE 停止）。在 Output Log 里截录所有 `[K1]` 日志，写出**完整顺序**。

6. 在 EndPlay 的日志里打印 `EndPlayReason`（枚举转字符串 `UEnum::GetValueAsString`），确认 PIE 停止触发的原因是 `EEndPlayReason::EndPlayInEditor`，还是 `EEndPlayReason::Destroyed`。

### 进阶任务

- 加入 `OnConstruction(const FTransform& Transform)` override，在里面打印日志。在 Editor 里移动 Actor 的位置，观察是否触发（Construction Script 在 Editor 中拖动 Actor 时每次变换都会重新执行）。
- 在构造函数里尝试调用 `GetWorld()`，观察返回值是否为 null（CDO 阶段 World 不存在）；在 `PostInitializeComponents` 里调用 `GetWorld()`，观察是否有效。
- 在 `BeginPlay` 之前（例如 `PostInitializeComponents`）调用 `GetNetMode()`，记录值；再在 PIE 多人模式（Players = 2）下运行，比较两端的 `NetMode`（`ENetMode::NM_Client` vs `ENetMode::NM_ListenServer`）。
- 阅读 `Actor.cpp` 第 4753 行的 `AActor::BeginPlay()` 实现，回答：BeginPlay 里对 Component 做了什么（提示：`Component->BeginPlay()`），顺序是 Actor BeginPlay 先还是 Component BeginPlay 先？

### 验收点

- Output Log 里日志顺序与预期完全一致：构造函数 → `PreInitializeComponents` → `PostInitializeComponents` → `BeginPlay` → `Tick`（多次）→ `EndPlay` → `Destroyed`。
- 能说清楚：构造函数里 `GetWorld()` 为何可能返回 null，而 `BeginPlay` 里一定不为 null（在 Play 流程中）。
- 能说清楚 `EndPlay` 和 `Destroyed` 的调用时机差异：`EndPlay` 在 Actor 停止参与游戏逻辑时调用（PIE 停止、`Destroy()` 被调用均会触发），`Destroyed` 在 Actor 被从 World 移除前的最后广播点（内部调用 `RouteEndPlay`）。
- 四固定问题书面回答完整（见"复盘问题"节）。

### 观察点

- **构造函数 vs BeginPlay 能做的事**：
  - 构造函数：可以 `CreateDefaultSubobject`，设置成员默认值，注册 Component。此时 World 不存在，`GetWorld()` 返回 null（CDO 阶段）或返回持久 World（在 Editor 里拖入场景时），但游戏尚未开始，没有其他 Actor、没有物理状态、没有 Net 角色。
  - `PostInitializeComponents`：所有 Component 已经 `RegisterComponent`，可以访问 Component 数据和做初始跨组件配置。
  - `BeginPlay`：World 的 gameplay 已经开始，可以安全地查询其他 Actor、访问 GameMode、GameState、调用物理、播放音效。这是"游戏执行真正开始"的时刻。
- `Tick` 默认每帧在 GameThread 上调用。在构造函数里可以用 `PrimaryActorTick.bCanEverTick = true` 开启，`false` 关闭——关闭可节省性能。
- `Destroyed()` 内部第一件事是调用 `RouteEndPlay(EEndPlayReason::Destroyed)`，再广播 `OnDestroyed` delegate，再调用 `ReceiveDestroyed()`（Blueprint 侧）。EndPlay 和 Destroyed 之间有调用栈嵌套关系，不是并列关系。

### 常见坑

- **在构造函数里调用 `GetWorld()->SpawnActor`**：CDO 构造期 World 可能为 null 或是编辑器的"持久 Editor World"而非游戏 World，逻辑会产生非预期行为。正确位置是 `BeginPlay`。
- **忘记 `Super::BeginPlay()`**：UE 用"路由"机制（`ReceiveBeginPlay`/`ReceiveEndPlay`）把 Blueprint 侧的事件和 C++ 侧串联，如果不调用 `Super::BeginPlay()`，Component 的 `BeginPlay` 不会被调用（见 `Actor.cpp:4764`，BeginPlay 里遍历并调用每个 Component 的 BeginPlay）。
- **重写 `Destroyed` 却没有 `EndPlay` 清理**：`EndPlay` 不仅在销毁时触发，PIE 停止时也触发。资源清理（比如停止定时器、取消订阅委托）应放在 `EndPlay`，不要只放在 `Destroyed`。
- **在 `EndPlay` 里访问 `GetWorld()`**：`EndPlayReason == EEndPlayReason::LevelTransition` 时，World 可能正在被销毁，某些 World 子系统已经不可用，谨慎调用。

### 复盘问题（四固定问题 + 题目特异问题）

- **Q1 — 执行真正开始时刻**：游戏逻辑的执行真正开始于 `BeginPlay`。构造函数和 `PostInitializeComponents` 是初始化阶段，不是"gameplay 执行"阶段。`Tick` 每帧调用，是持续执行阶段。
- **Q2 — 生命周期拥有者**：Actor 由 `ULevel`（通过 Outer 链）和 `UWorld`（通过 `Level::Actors` TArray 以及 World 的 UPROPERTY 链）共同持有。`UWorld::DestroyActor` 会把 Actor 从 Level 的 Actors 数组里移除，并标记 Actor 为 PendingKill，下次 GC 时真正回收。
- **Q3 — 涉及哪些 Named Thread**：`BeginPlay`、`Tick`、`EndPlay`、`Destroyed` 全部在 `GameThread` 上调用。不涉及 RenderThread 或 RHIThread（除非你在这些钩子里主动 `ENQUEUE_RENDER_COMMAND`）。
- **Q4 — GC 可见性**：Actor 通过 Outer = `ULevel` 挂在 World 链上，GC 从 `UEngine` 根集出发，沿 Outer 链可达。Actor 内部的 Component 成员（通过 `CreateDefaultSubobject` 创建）以 Actor 为 Outer，自动 reachable。如果你在 Actor 里新增一个 `UPROPERTY` 标注的 `TObjectPtr<UActorComponent>` 成员，这条 UPROPERTY 引用也确保 GC 可见。
- **题目特异**：为什么在 Editor 里把 Actor 拖入场景后，构造函数会被多次调用？（CDO 构造一次，每次拖入实例化一次，每次在 Editor 内改属性可能触发 `OnConstruction` 重新走一次）。

### 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h`（`AActor` 类声明，`BeginPlay`/`EndPlay`/`Destroyed` 声明）
- `Engine/Source/Runtime/Engine/Private/Actor.cpp`，第 4753 行 `AActor::BeginPlay()`，第 3232 行 `AActor::EndPlay()`，第 3284 行 `AActor::Destroyed()`，第 6544 行 `AActor::PostInitializeComponents()`，第 4249 行 `AActor::PostSpawnInitialize()`，第 4347 行 `AActor::FinishSpawning()`，第 4403 行 `AActor::PostActorConstruction()`，第 4690 行 `AActor::DispatchBeginPlay()`

---

## 练习 K2 — Actor + Component 组合与 SceneComponent transform 层级

### 目标

掌握 `USceneComponent` 的 transform 附着（attach）层级，理解 `CreateDefaultSubobject`（构造期）与运行时 `RegisterComponent` 的区别；能在运行时正确添加动态 Component。

### 前置理解

- 完成 K1，理解 Actor 构造函数与 `BeginPlay` 的区别。
- 知道 `UPROPERTY` 对 GC 的作用。
- 了解 `FTransform`（位置/旋转/缩放三合一）。

### 必做任务

1. 派生一个新 `AActor`，类名建议 `AComponentDemoActor`。

2. 在构造函数里用 `CreateDefaultSubobject<USceneComponent>` 创建 RootComponent，再创建两个子 `USceneComponent`（命名为 `ChildA` 和 `ChildB`），用 `SetupAttachment(RootComponent)` 把它们附着到 Root。打印日志确认此时 `GetWorld()` 是否为 null（构造期）。

3. 在 `BeginPlay` 里：
   - 打印 `RootComponent->GetComponentLocation()`（即 Actor 的世界位置）。
   - 把 `ChildA` 的相对位置设为 `(100, 0, 0)`，打印 `ChildA->GetComponentLocation()`（世界位置应为 Root 位置 + 偏移）。
   - 打印 `GetActorLocation()`，确认它和 `RootComponent->GetComponentLocation()` 一致。

4. 在 `BeginPlay` 里演示运行时动态添加 Component：
   - 用 `NewObject<USceneComponent>(this, TEXT("DynamicChild"))` 创建一个新 Component（Outer = this Actor）。
   - 调用 `DynamicChild->SetupAttachment(RootComponent)` 设置附着（此处在运行期，不是构造期）。
   - 调用 `DynamicChild->RegisterComponent()` 向 World 注册。
   - 打印 `DynamicChild->IsRegistered()` 确认注册成功。

5. 新建另一个不含 transform 的纯逻辑 Component：派生 `UActorComponent`（不是 USceneComponent），类名 `ULogicOnlyComponent`，在其 `BeginPlay` 里打印一条日志。在 `AComponentDemoActor` 构造函数里用 `CreateDefaultSubobject` 添加它。在 `BeginPlay` 里打印 `LogicComponent->GetComponentLocation()` 的结果——预期：**编译报错**，因为 `UActorComponent` 没有 `GetComponentLocation`（非 SceneComponent，无 transform），这个编译错误本身就是练习的目标观察点。

### 进阶任务

- 阅读 `ActorComponent.cpp` 第 1542 行 `UActorComponent::OnRegister()` 和第 1923 行 `UActorComponent::RegisterComponentWithWorld()`，理解 RegisterComponent 做了哪几件事（设置 World 指针、触发 `OnRegister` 回调、通知 Owner Actor）。
- 对比 `USceneComponent::SetupAttachment`（`SceneComponent.h` 中声明）与 `USceneComponent::AttachToComponent`（声明同处）：两者的差异是 `SetupAttachment` 只设置 `AttachParent` 指针，不实际执行 transform 更新（适合构造期，此时 Component 尚未注册）；`AttachToComponent` 执行完整的 transform 接管，适合运行期已注册的 Component 之间的重新附着。
- 在 `EndPlay` 里用 `DynamicChild->DestroyComponent()` 显式销毁动态添加的 Component，观察 Component 的 `OnUnregister` 是否被调用（在 `ULogicOnlyComponent` 里 override `OnUnregister` 并打印日志）。
- 阅读 `AActor::GetActorLocation()` 的实现——它内部直接调用 `RootComponent->GetComponentLocation()`，验证"Actor 的 transform 由 RootComponent 决定"这一设计。

### 验收点

- `SetupAttachment` 在构造函数里调用不报错，`AttachToComponent` 在构造函数里调用可以工作但语义上错误（此时 Component 未注册），理解两者差异。
- 运行时动态添加的 Component 在调用 `RegisterComponent()` 之前，`IsRegistered()` 返回 false，不参与 Tick、Render 等流程。
- 确认 `UActorComponent`（非 SceneComponent）没有 `GetComponentLocation` 方法，理解 "SceneComponent = ActorComponent + Transform"这一设计分层。
- 四固定问题书面回答完整。

### 观察点

- **Component 层级结构**：UE 把 Actor 的能力拆成 Component 插件。渲染、碰撞、音效、寻路——各是独立 Component。Actor 本身是"空壳 + 协调者"，transform 由 RootComponent 代理。
- **`CreateDefaultSubobject` 的特殊性**：只能在构造函数（包括 `FObjectInitializer` 回调）里调用，产生一个以 Actor 为 Outer、被 `UPROPERTY` 自动跟踪的子对象。这是构造期 GC 安全性的保证。在 `BeginPlay` 或其他运行期函数里调用 `CreateDefaultSubobject` 会 check 失败（引擎有断言保护）。
- **非 SceneComponent 不参与 transform 层级**：`UActorComponent` 子类（如 `UMovementComponent`、自定义逻辑组件）不挂在 SceneComponent 的父子树上，它们不影响 Actor 的空间位置，也没有世界坐标。

### 常见坑

- **运行期添加 Component 忘记调用 `RegisterComponent()`**：`NewObject` 只是在内存里创建对象，没有注册到 World，Component 的 `BeginPlay` 和 `Tick` 都不会被调用。
- **运行期用 `SetupAttachment` 而不是 `AttachToComponent`**：`SetupAttachment` 在运行期能编译通过，但它不更新 transform 继承，你会看到子 Component 位置不跟随 Parent 移动。
- **在 Outer 错误的情况下 `RegisterComponent`**：如果用 `NewObject<USceneComponent>(SomeNonActorObject, ...)` 创建了 Component，再尝试 `RegisterComponent()`，引擎会 check `GetOwner()` 不为 null（Owner = Cast<AActor>(GetOuter())），Outer 不对会在这里 crash。
- **把 `TObjectPtr<USceneComponent> ChildA` 成员变量不加 `UPROPERTY`**：GC 遍历时看不见它，ChildA 可能在下次 GC 运行时被回收（即使 Actor 本身活着），后续访问产生悬空指针。

### 复盘问题

- **Q1 — 执行真正开始时刻**：`SetupAttachment`（构造函数阶段，Component 注册前）；`RegisterComponent()`（运行期，Component 正式加入 World，随后触发 `OnRegister` 回调）；Component 的 `BeginPlay` 在 Actor 的 `BeginPlay` 内被调用（`Actor.cpp:4764`）。
- **Q2 — 生命周期拥有者**：`CreateDefaultSubobject` 创建的 Component 以 Actor 为 Outer，GC 通过 Outer 链 reachable；运行期 `NewObject` 创建的 Component 同样以 Actor 为 Outer（必须显式指定），GC 可达前提是 Outer 链正确且 `UPROPERTY` 或 Outer 链上有强引用。
- **Q3 — 涉及哪些 Named Thread**：`RegisterComponent`、`OnRegister`、Component `BeginPlay` 全部在 GameThread 上调用。Render Proxy 的创建（如果是 `UPrimitiveComponent`）会从 GameThread 投递到 RenderThread，但本练习只用 `USceneComponent`，不涉及渲染线程。
- **Q4 — GC 可见性**：Component 成员必须加 `UPROPERTY`（或使用 `TObjectPtr<T>` + `UPROPERTY`），同时 Component 的 Outer 必须是 Actor，才能确保双通道 GC 可见。
- **题目特异**：为什么 `AttachToComponent` 在构造期调用会有问题？（Component 未向 World 注册，RegisterComponent 之后才有完整的 transform 状态；构造期调用时内部的 transform 更新代码可能触发对 World 的访问，而此时 World 指针尚未设置）

### 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/Components/ActorComponent.h`（`UActorComponent` 基类声明）
- `Engine/Source/Runtime/Engine/Classes/Components/SceneComponent.h`（`USceneComponent`，`SetupAttachment`/`AttachToComponent` 声明）
- `Engine/Source/Runtime/Engine/Private/Components/ActorComponent.cpp`，第 1542 行 `OnRegister()`，第 1923 行 `RegisterComponentWithWorld()`

---

## 练习 K3 — UWorld / ULevel / UGameInstance 三层关系 + SpawnActor 路径

### 目标

通过阅读 `LevelActor.cpp` 的 `UWorld::SpawnActor` 实现，画出 World → Level → Actors 的对象链路图，并理解 `UGameInstance` 在关卡切换时扮演的"跨 World 存活容器"角色。

### 前置理解

- 完成模块 D 的 `NewObject` 章节，知道 `NewObject<T>(Outer, Class, ...)` 的第一个参数决定 Outer。
- 完成 K1，理解 Actor 的生命周期。
- 知道 PIE 模式下有多个 `UWorld` 实例（Server World / Client World）。

### 必做任务

1. 在 Editor 里打开 Output Log，启用 `LogSpawn Verbose`（在 Log 的 Filter 下找到 Spawn，或在控制台输入 `log LogSpawn Verbose`）。PIE 开始，在 GameMode::BeginPlay 里调用 `SpawnActor<AActor>`，观察 LogSpawn 的输出，确认以下字段出现：Actor 的类名、所在 Level 名称。

2. 阅读 `Engine/Source/Runtime/Engine/Private/LevelActor.cpp`，找到 `UWorld::SpawnActor(UClass*, FTransform const*, const FActorSpawnParameters&)` 实现（第 455 行起）。追踪以下关键步骤：
   - 第 532-536 行：如何确定 `LevelToSpawnIn`（优先用 `SpawnParameters.OverrideLevel`，其次用 `Owner->GetLevel()`，最后用 `CurrentLevel`）。
   - 第 671 行：`NewObject<AActor>(LevelToSpawnIn, Class, ...)` — 这里 `LevelToSpawnIn` 即新 Actor 的 Outer，直接决定 `actor->GetOuter() == LevelToSpawnIn`。
   - 第 738 行：`LevelToSpawnIn->TryAddActorToList(Actor, ...)` — Actor 被加入 `ULevel::Actors` 数组。
   - 第 754 行：`Actor->PostSpawnInitialize(...)` — 触发 `PreInitializeComponents`、`PostInitializeComponents`、若非 `bDeferConstruction` 则直接调用 `FinishSpawning`（含 `BeginPlay` 路由）。

3. 画一张 ASCII 图，展示以下关系：
   ```
   UGameInstance
     └─(UPROPERTY WorldList)─ UWorld (GameWorld)
         ├─ PersistentLevel (ULevel)
         │    └─ Actors: [AActor_0, AActor_1, ...]
         └─ StreamedLevels: [ULevelStreaming, ...]
              └─ (loaded) ULevel
                   └─ Actors: [...]
   ```
   标注每条边是 Outer 链还是 UPROPERTY 引用（或两者都是）。

4. 写 3-5 行说明：关卡切换（`OpenLevel`）时，`UWorld` 被销毁并重建，而 `UGameInstance` 在整个游戏会话中存活。结合 `01-心智模型` §6（Subsystem 化）解释：为什么跨关卡持久数据应放 `UGameInstanceSubsystem`，而不是放 `UWorldSubsystem`。

5. 进入 `UWorld::CreateWorld`（可通过 Grep 找到实现位置），看 GameInstance 与 World 的关联是如何建立的（`World->SetGameInstance(...)` 或等价调用），写一行观察记录。

### 进阶任务

- 阅读 `FActorSpawnParameters` 结构体（在 `Actor.h` 中搜索该结构体定义），找到 `Owner`、`Instigator`、`ObjectFlags`、`bDeferConstruction` 字段，分别用一句话描述用途。
- 理解 `bDeferConstruction = true` 的含义：`SpawnActorDeferred` 模式下，Actor 在 `SpawnActor` 返回时尚未调用 `PostInitializeComponents` 和 `BeginPlay`，需要调用方在完成自定义初始化后手动调用 `Actor->FinishSpawning(Transform)`。这在需要在初始化之前设置属性的情况下非常有用。
- 比较 PIE 模式和 Standalone 运行时 `UWorld` 创建路径的差异：PIE 时 `UEditorEngine::CreatePIEWorldByDuplication` 从 Editor World 复制出一份 PIE World；Standalone 时走 `UEngine::LoadMap` 流程，直接从磁盘加载 `UPackage`。

### 验收点

- 能完整描述 `SpawnActor` 的关键步骤（LevelToSpawnIn 选择 → `NewObject` 创建 Actor + 设置 Outer → `TryAddActorToList` → `PostSpawnInitialize`），能指出每步在源码的行号。
- 画出的对象链路图中，Outer 链和 UPROPERTY 引用边都有标注，且方向正确。
- 能清晰说明"在 `BeginPlay` 前调用 `GetWorld()` 可能返回非预期 World"的原因（Actor 的 Outer 链决定 `GetWorld()`，如果 Actor 是在 Editor World 而非 Game World 里创建的，返回的是 Editor World）。
- 四固定问题书面回答完整。

### 观察点

- **`UWorld` 并不是 UObject 的根**：`UWorld` 本身的 Outer 可以是 `UPackage`（序列化场景时）或 `UGameInstance`（运行时）。GC 根集不是 `UWorld` 直接——而是 `GEngine`（加 `AddToRoot`）→ `UGameInstance` → `UWorld` 这条链。
- **`ULevel::Actors` 是 `TArray<AActor*>`，且是 `UPROPERTY`**：这是 GC 扫描 Actor 的另一条路径，除了 Outer 链之外，`UPROPERTY TArray<AActor*> Actors` 也让 GC 通过 ULevel 直接扫描到所有 Actor。两条路径互为冗余，双重保险。
- **PIE 中有多个 UWorld**：Editor World（编辑器本身的世界，始终存在）+ PIE World（每次 PIE 启动创建，停止销毁）。如果你的代码在 Editor World 里创建了 Actor，它会在 PIE 停止时依然存在，这是 PIE 模式下很难排查的 Bug 来源。
- **`StreamedLevels` 里的 Actor 归属**：Level Streaming 加载的 sub-level 有自己的 `ULevel` 实例，其 Actor 的 Outer 是那个 sub-level 的 `ULevel`，而不是 PersistentLevel。`GetWorld()` 对这些 Actor 仍然有效（所有 Level 共享同一个 World），但 `GetLevel()` 会返回各自所属的 `ULevel`。

### 常见坑

- **在非 GameThread 上调用 `GetWorld()`**：`AActor::GetWorld()` 假设在 GameThread 调用，跨线程调用会产生数据竞争。如果你在 `UE::Tasks::Launch` 里持有 Actor 指针并调用 `GetWorld()`，需要先在 GameThread 上提取好 `UWorld*`，按值捕获传给工作线程（类比 ENQUEUE_RENDER_COMMAND 的值捕获原则）。
- **`GetWorld()` 在模块 `StartupModule` 里调用**：Module 加载阶段（早于任何 World 创建），`GWorld` 为 null，`GetWorld()` 返回 null。

### 复盘问题

- **Q1 — 执行真正开始时刻**：`TryAddActorToList` 把 Actor 加入 Level 发生在 `NewObject` 之后（LevelActor.cpp 第 738 行）。`PostSpawnInitialize` 才是 Actor 初始化钩子序列的起点（第 754 行）。
- **Q2 — 生命周期拥有者**：Actor 由 `ULevel`（Outer 链 + `UPROPERTY TArray<AActor*> Actors`）共同持有。`ULevel` 由 `UWorld`（`UPROPERTY ULevel* PersistentLevel`）持有。`UWorld` 由 `UGameInstance`（`UPROPERTY`）持有。链条最终归根于 `GEngine`（AddToRoot）。
- **Q3 — 涉及哪些 Named Thread**：`SpawnActor` 在 GameThread 上调用，`NewObject`、`TryAddActorToList`、`PostSpawnInitialize` 全部在 GameThread 执行。
- **Q4 — GC 可见性**：Actor 通过 Outer = ULevel 和 `ULevel::Actors UPROPERTY` 两条路径 reachable；ULevel 通过 `UWorld::PersistentLevel UPROPERTY` reachable；整条链归根于 GEngine 根集。

### 对应官方参考

- `Engine/Source/Runtime/Engine/Private/LevelActor.cpp`，第 455 行 `UWorld::SpawnActor`，第 532-538 行（LevelToSpawnIn 选择），第 671 行（NewObject），第 738 行（TryAddActorToList），第 754 行（PostSpawnInitialize）
- `Engine/Source/Runtime/Engine/Classes/Engine/World.h`（`UWorld` 类，`PersistentLevel`、`StreamedLevels`、`GameInstance` 字段声明）
- `Engine/Source/Runtime/Engine/Classes/Engine/GameInstance.h`（`UGameInstance` 类，`GetSubsystem<T>()` 接口）

---

## 练习 K4 — 自定义 UGameInstanceSubsystem + 三层 Subsystem 生命周期对比

### 目标

实现一个 `UGameInstanceSubsystem`，观察其生命周期与 Actor、World 的关系；对比三层 Subsystem（Engine / GameInstance / World）在 PIE 多次开始/停止时的行为差异；理解为什么 Subsystem 是 UE 中"生命周期绑定的单例"的规范实现。

### 前置理解

- 完成 K1-K3，理解 Actor 生命周期与 World 层级。
- 重读 `01-心智模型` §6（Subsystem 化，而非全局单例）。
- 知道 `UObject` 的 Outer 链与 GC 可见性。

### 必做任务

1. 派生 `UGameInstanceSubsystem`，类名建议 `UMyGameSubsystem`。声明头文件如下结构：

   ```cpp
   UCLASS()
   class UMyGameSubsystem : public UGameInstanceSubsystem
   {
       GENERATED_BODY()
   public:
       virtual void Initialize(FSubsystemCollectionBase& Collection) override;
       virtual void Deinitialize() override;
   
       // 业务逻辑示例：跨关卡计数器
       int32 GetPlayCount() const { return PlayCount; }
       void IncrementPlayCount() { PlayCount++; }
   
   private:
       int32 PlayCount = 0;
   };
   ```

2. 在 `Initialize` 和 `Deinitialize` 里各打印一条带时间戳的日志（用 `FDateTime::Now().ToString()`），证明它们的调用时机。

3. 在 `ALifecycleActor::BeginPlay()`（K1 的 Actor，或新建一个测试 Actor）里，通过以下方式访问 Subsystem：
   ```cpp
   if (UGameInstance* GI = GetGameInstance())
   {
       if (UMyGameSubsystem* Sys = GI->GetSubsystem<UMyGameSubsystem>())
       {
           Sys->IncrementPlayCount();
           UE_LOG(LogTemp, Warning, TEXT("[K4] PlayCount = %d"), Sys->GetPlayCount());
       }
   }
   ```
   PIE 开始，观察 PlayCount 被递增；PIE 停止，再 PIE 开始，观察 PlayCount 是否从 0 重新开始（应该是，因为每次 PIE 创建新的 GameInstance）。

4. 新建一个 `UWorldSubsystem` 派生类 `UMyWorldSubsystem`，override `OnWorldBeginPlay` 和 `OnWorldEndPlay`，各打印日志。在 Editor 里开两个 PIE Player（Players = 2），观察 `UMyWorldSubsystem` 是否有两个独立实例被初始化（Server World + Client World 各一个）。

5. 新建一个 `UEngineSubsystem` 派生类 `UMyEngineSubsystem`，override `Initialize` 和 `Deinitialize`，打印日志。多次开始/停止 PIE，确认 `UMyEngineSubsystem::Initialize` 只在 Editor 启动时调用一次，`Deinitialize` 只在 Editor 关闭时调用（不随 PIE 重置）。

6. 对比三者日志，填写以下表格（作为观察记录交付）：

   | Subsystem 类型 | `Initialize` 触发时机 | `Deinitialize` 触发时机 | PIE 两次时实例数量 |
   |---|---|---|---|
   | `UEngineSubsystem` | | | |
   | `UGameInstanceSubsystem` | | | |
   | `UWorldSubsystem` | | | |

### 进阶任务

- 阅读 `Engine/Source/Runtime/Engine/Public/Subsystems/Subsystem.h`：`USubsystem::ShouldCreateSubsystem(UObject* Outer)` 返回 false 时，该 Subsystem 不会被自动实例化。在 `UMyWorldSubsystem` 里 override 它，对 `EWorldType::Editor` 类型的 World 返回 false（避免在 Editor World 里创建实例），观察 World Outliner 里的变化。
  
  `UWorldSubsystem` 的 `DoesSupportWorldType` 函数（`WorldSubsystem.h` 第 61 行）提供了更细粒度的控制：只对特定 `EWorldType`（`Game`、`PIE`、`Editor` 等）创建实例，是过滤无关 World 的推荐方式。

- 理解 **CDO 机制与 Subsystem 的自动发现**：UE 框架在初始化 Subsystem 集合（`FSubsystemCollection`）时，用反射遍历所有 `UClass` 中派生自对应 Subsystem 基类的类，对每个 `ShouldCreateSubsystem` 返回 true 的类，调用 `NewObject` 创建实例，再调用 `Initialize`。这整个过程不需要你手动注册——`UCLASS()` 宏 + UHT 生成的反射元数据自动完成了"告诉 UE 这个类存在"的工作。这是 UHT 反射系统（Mindset Shift 2）在 Subsystem 上的直接应用。

- 在 `UGameInstanceSubsystem` 里尝试 override `BeginDestroy()`（`UObject::BeginDestroy` 是 GC 触发销毁时的钩子）并打印日志。对比 `Deinitialize` 和 `BeginDestroy` 的调用顺序。文档建议：**不应该** override `BeginDestroy` 做业务逻辑清理，因为在 `BeginDestroy` 被调用时 Subsystem 已经进入 GC 回收流程，其他 UObject 引用可能已经失效。正确的清理位置是 `Deinitialize`。

### 验收点

- `UMyEngineSubsystem` 在 Editor 生命周期内只 Initialize 一次，不随 PIE 重置——验证"生命周期绑 Engine"的含义。
- `UMyGameInstanceSubsystem` 每次 PIE 启动新建一个实例，PIE 停止销毁——验证"生命周期绑 GameInstance"。
- `UMyWorldSubsystem`（PIE Players = 2 时）有两个独立实例（Server World 一个，Client World 一个）——验证"生命周期绑 World，每 World 独立"。
- 能正确用 `GetGameInstance()->GetSubsystem<T>()` 获取 Subsystem 实例（而非用全局变量或裸单例）。
- 四固定问题书面回答完整。

### 观察点

- **Subsystem = 由 UE 框架托管生命周期的单例 UObject**：它不是裸全局，不是 Meyer's singleton，也不是手动注册的管理器——它是一个普通 `UCLASS`，UE 的反射系统在正确时机自动创建和销毁它。你只需要继承对应基类，框架做其余一切。
- **与 Mindset Shift 4（GC ownership）的联动**：Subsystem 是 UObject，它的生命周期由 GC 管理。但 GC 不会"主动"提前销毁它——因为它的 Outer 是宿主容器（GameInstance/World），只要容器活着，Subsystem 就沿 Outer 链 reachable。`Deinitialize` 是框架在容器销毁前主动调用的清理钩子，在 GC 真正回收前完成业务级清理。
- **"全局唯一"的正确做法**：当你需要一个进程内全局唯一的对象，不要写 `static MyManager* GMyManager = nullptr`，而是派生 `UEngineSubsystem`。当你需要跨关卡但游戏会话内唯一，派生 `UGameInstanceSubsystem`。当你需要跟某个具体 World 绑定，派生 `UWorldSubsystem`。这三个选择覆盖了绝大多数"单例等价物"的需求。
- **`UWorldSubsystem::DoesSupportWorldType`**（WorldSubsystem.h 第 61 行）：默认实现只对 `EWorldType::Game` 和 `EWorldType::PIE` 返回 true，过滤掉 Editor World、Preview World 等不需要游戏逻辑的 World 类型。如果你的 Subsystem 有 Editor 功能，override 此函数时要小心不要让它在 Game World 里缺失。

### 常见坑

- **在 Subsystem 里持有 `UWorld*` 成员变量不加 `UPROPERTY`**：Subsystem 的 Outer 是 GameInstance，GameInstance 的 UPROPERTY 链不经过 World，如果你在 `UGameInstanceSubsystem` 里直接缓存 `UWorld* CurrentWorld`，GC 看不见这个指针，World 销毁后访问是悬空指针。正确做法：用 `UPROPERTY` 标记，或在每次使用时通过 `GetGameInstance()->GetWorld()` 即时获取。
- **在 `UWorldSubsystem` 的 `Initialize` 里调用 `GetWorld()` 期望获取 GameWorld**：`Initialize` 的调用时机早于 `OnWorldBeginPlay`，此时 World 可能尚未完全初始化（GameMode/GameState 尚未创建）。需要 gameplay 相关数据的初始化应放在 `OnWorldBeginPlay`。
- **多个 PIE 实例的 `UEngineSubsystem` 被多次初始化的错觉**：UEngineSubsystem 绑定 GEngine，整个 Editor 进程只有一个 `GEngine` 实例，所以 `UEngineSubsystem::Initialize` 确实只调用一次。如果你看到多次日志，可能是你把 Subsystem 误放成了 GameInstanceSubsystem 或 WorldSubsystem。
- **不检查 `GetSubsystem<T>()` 的返回值**：如果 `ShouldCreateSubsystem` 返回了 false，`GetSubsystem<T>()` 返回 null，直接解引用会 crash。养成习惯：总是先 null check。

### 复盘问题

- **Q1 — 执行真正开始时刻**：Subsystem 的 `Initialize` 在 `FSubsystemCollection::Initialize` 里被调用，发生在 GameInstance 创建（`UGameInstanceSubsystem`）或 World 创建（`UWorldSubsystem`）之后，但在 `BeginPlay` 之前。`OnWorldBeginPlay`（`UWorldSubsystem`）在 World 开始 gameplay 时调用，即 Actor 的 `BeginPlay` 序列启动前。
- **Q2 — 生命周期拥有者**：`UGameInstanceSubsystem` 的 Outer 是 `UGameInstance`，GC 通过 Outer 链 reachable；`UWorldSubsystem` 的 Outer 是 `UWorld`；`UEngineSubsystem` 的 Outer 是 `GEngine`（AddToRoot 的根集对象）。
- **Q3 — 涉及哪些 Named Thread**：`Initialize`、`Deinitialize`、`OnWorldBeginPlay` 全部在 GameThread 上调用。Subsystem 本身不涉及 RenderThread 或 RHIThread（除非 Subsystem 内部主动投递渲染命令）。
- **Q4 — GC 可见性**：Subsystem 是 UObject，通过 Outer 链（Outer = 宿主容器）reachable，不需要额外的 `AddToRoot`。Subsystem 内部成员若是 `UObject*`，必须加 `UPROPERTY` 才能被 GC 看见。
- **题目特异**：CDO 机制和 Subsystem 自动实例化的关系是什么？（`ShouldCreateSubsystem` 在 CDO 上被调用，用 CDO 做创建前判断；实际 Subsystem 实例通过 `NewObject` 创建，是普通实例不是 CDO）。为什么 Subsystem 不应该 override `BeginDestroy` 做清理逻辑？

### 对应官方参考

- `Engine/Source/Runtime/Engine/Public/Subsystems/Subsystem.h`（`USubsystem` 基类，`Initialize`/`Deinitialize`/`ShouldCreateSubsystem` 声明）
- `Engine/Source/Runtime/Engine/Public/Subsystems/GameInstanceSubsystem.h`（`UGameInstanceSubsystem`，`UCLASS(Abstract, Within=GameInstance)`）
- `Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h`（`UWorldSubsystem`，`DoesSupportWorldType`/`OnWorldBeginPlay`/`OnWorldEndPlay`）
- `Engine/Source/Runtime/Engine/Public/Subsystems/EngineSubsystem.h`（`UEngineSubsystem`，派生自 `UDynamicSubsystem`）
- `Engine/Source/Runtime/Engine/Classes/Engine/GameInstance.h`（`UGameInstance::GetSubsystem<T>()` 接口）

---

## 对象生命周期图（触发规则：K 模块）

以下图展示一个 `AActor` 从 `SpawnActor` 到 GC 回收的完整弧，标出 Outer 链和关键事件时刻。

```
UEngine (AddToRoot, GC 根集)
  │
  └─[UPROPERTY]─ UGameInstance (Outer = UEngine)
       │               │
       │         [FSubsystemCollection]
       │               └─ UGameInstanceSubsystem (Outer = UGameInstance)
       │                      ↑ Initialize()        ↑ Deinitialize()
       │
       └─[UPROPERTY WorldList]─ UWorld (Outer = UGameInstance)
             │                        │
             │                 [FSubsystemCollection]
             │                        └─ UWorldSubsystem (Outer = UWorld)
             │                               ↑ OnWorldBeginPlay()
             │
             ├─[UPROPERTY PersistentLevel]─ ULevel (Outer = UWorld)
             │       │
             │       └─[UPROPERTY Actors TArray]─ AActor (Outer = ULevel)
             │               │                        ↑
             │               │          SpawnActor: NewObject(LevelToSpawnIn, ...)
             │               │          → TryAddActorToList
             │               │          → PostSpawnInitialize
             │               │            ├─ PreInitializeComponents
             │               │            ├─ PostInitializeComponents
             │               │            └─ (if !bDeferred) FinishSpawning → BeginPlay
             │               │
             │               │          每帧: Tick (GameThread)
             │               │
             │               │          销毁: DestroyActor
             │               │            ├─ EndPlay (EEndPlayReason::Destroyed)
             │               │            ├─ Destroyed()
             │               │            ├─ 从 Level::Actors 移除
             │               │            └─ GC 下次运行时回收
             │               │
             │               └─[UPROPERTY / Outer]─ UActorComponent (Outer = AActor)
             │                       ↑ RegisterComponent → OnRegister → BeginPlay
             │                       ↑ DestroyComponent → OnUnregister → EndPlay
             │
             └─[UPROPERTY StreamedLevels]─ ULevelStreaming
                     └─ (loaded) ULevel
                           └─ Actors: [...]
```

**Outer 链 vs UPROPERTY 引用**：上图中 `─[Outer]─` 表示 Outer 链（`GetOuter()` 返回），`─[UPROPERTY]─` 表示显式 UPROPERTY 成员引用。两条路径都让 GC 可达，任何一条断了，对象就暴露在 GC 回收风险下。

---

## 做完本模块后你现在应该能说清楚什么

做完模块 K 的四道练习后，至少能把以下问题直接说清楚（不查文档）：

**关于 Actor 生命周期**：

- 构造函数阶段 `GetWorld()` 可能为 null，`BeginPlay` 阶段 `GetWorld()` 保证有效（在 PIE/Game 中）——原因是 Outer 链在 SpawnActor 完成后才正确建立，而 BeginPlay 在此之后被调用。
- `EndPlay` 和 `Destroyed` 的调用嵌套关系（`Destroyed` 内部调用 `RouteEndPlay`），以及为什么资源清理应放 `EndPlay` 而非 `Destroyed`。
- `Tick` 只在 `PrimaryActorTick.bCanEverTick = true` 时调用，且始终在 GameThread 上。

**关于 Component 组合**：

- `CreateDefaultSubobject` 只能在构造函数里调用；运行时添加 Component 必须 `NewObject + RegisterComponent`。
- `SetupAttachment`（构造期，只设 Parent 指针）vs `AttachToComponent`（运行期，执行完整 transform 接管）。
- `UActorComponent` 无 transform，`USceneComponent` 在 `UActorComponent` 上加 transform + 附着，`UPrimitiveComponent` 再加渲染/碰撞。

**关于 World / Level / GameInstance**：

- `SpawnActor` 内部：`NewObject<AActor>(LevelToSpawnIn, ...)` 设置 Outer，`TryAddActorToList` 把 Actor 加入 `ULevel::Actors`，`PostSpawnInitialize` 触发钩子序列。
- 关卡切换时 `UWorld` 销毁重建，`UGameInstance` 跨关卡存活；跨关卡持久状态用 `UGameInstanceSubsystem`。

**关于 Subsystem 体系**：

- 三层生命周期锚点：Engine（进程全程）/ GameInstance（一次游戏会话）/ World（每个 World 实例独立）。
- 自动实例化机制：`UCLASS()` + UHT 反射让框架在正确时机自动发现并创建 Subsystem 实例，无需手动注册。
- 为什么裸全局单例在 PIE 场景下不安全：第一次 PIE 创建的 World 指针在第二次 PIE 时变成悬空指针，而 `UWorldSubsystem` 随 World 创建/销毁，每次都是全新实例。

---

## 本模块覆盖的 UE 源码清单

| 文件路径 | 关键内容 | 本模块用途 |
|---|---|---|
| `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h` | `AActor` 类声明，生命周期函数声明，`RootComponent`，`PrimaryActorTick` | K1/K2 Actor 生命周期与 Component 附着 |
| `Engine/Source/Runtime/Engine/Private/Actor.cpp` | `PostSpawnInitialize`（第 4249 行）、`FinishSpawning`（第 4347 行）、`PostActorConstruction`（第 4403 行）、`DispatchBeginPlay`（第 4690 行）、`BeginPlay`（第 4753 行）、`EndPlay`（第 3232 行）、`Destroyed`（第 3284 行）、`PostInitializeComponents`（第 6544 行）实现 | K1 钩子顺序观察；K3 SpawnActor 路径追踪 |
| `Engine/Source/Runtime/Engine/Classes/Components/ActorComponent.h` | `UActorComponent` 基类，`RegisterComponent`/`UnregisterComponent` 声明 | K2 Component 注册流程 |
| `Engine/Source/Runtime/Engine/Classes/Components/SceneComponent.h` | `USceneComponent`，`SetupAttachment`/`AttachToComponent` 声明，`EUpdateTransformFlags` | K2 transform 层级 |
| `Engine/Source/Runtime/Engine/Private/Components/ActorComponent.cpp` | `OnRegister`（第 1542 行）、`RegisterComponentWithWorld`（第 1923 行）实现 | K2 运行时 RegisterComponent 路径 |
| `Engine/Source/Runtime/Engine/Private/LevelActor.cpp` | `UWorld::SpawnActor` 实现（第 455 行起），`NewObject<AActor>(LevelToSpawnIn,...)`（第 671 行），`TryAddActorToList`（第 738 行），`PostSpawnInitialize`（第 754 行）| K3 SpawnActor 路径追踪 |
| `Engine/Source/Runtime/Engine/Classes/Engine/World.h` | `UWorld` 类，`PersistentLevel`，`StreamedLevels`，`GetSubsystem<T>()` | K3 World 层级 |
| `Engine/Source/Runtime/Engine/Classes/Engine/GameInstance.h` | `UGameInstance` 类，`GetSubsystem<T>()` | K4 Subsystem 访问 |
| `Engine/Source/Runtime/Engine/Public/Subsystems/Subsystem.h` | `USubsystem` 基类，`Initialize`/`Deinitialize`/`ShouldCreateSubsystem` | K4 Subsystem 基础 |
| `Engine/Source/Runtime/Engine/Public/Subsystems/GameInstanceSubsystem.h` | `UGameInstanceSubsystem`，`UCLASS(Abstract, Within=GameInstance)` | K4 GameInstance 生命周期 |
| `Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h` | `UWorldSubsystem`，`DoesSupportWorldType`，`OnWorldBeginPlay`，`UTickableWorldSubsystem` | K4 World 生命周期 |
| `Engine/Source/Runtime/Engine/Public/Subsystems/EngineSubsystem.h` | `UEngineSubsystem`，`UDynamicSubsystem` | K4 Engine 生命周期 |

---

## 指向结课项目

Cap1 将要求 ≥1 `AActor` 承载可视化逻辑 + ≥1 `UWorldSubsystem` 管理批处理状态，两者串联——这正是本模块 K1 + K4 能力的直接整合。做完本模块后，你已经具备了在 Cap1 里正确实现这两个约束所需的全部心智模型：Actor 的生命周期钩子、Component 的注册机制、World/Level 的 Outer 链、以及 Subsystem 的自动实例化与生命周期绑定。
