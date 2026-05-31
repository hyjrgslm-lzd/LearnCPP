> 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-1

## 目标

观察 `AActor` 从 Spawn 到销毁经历的五个生命周期钩子（`PostInitializeComponents` / `BeginPlay` / `Tick` / `EndPlay` / `Destroyed`）的精确调用顺序，理解每个钩子调用时 World 状态与 Component 注册状态的差异，建立"构造函数 ≠ BeginPlay"的清晰心智模型。

## 前置理解

- 已完成模块 D（`NewObject` / `UPROPERTY` / GC 根集），理解 UObject 生命周期。
- 知道 `UCLASS` / `GENERATED_BODY()` / `UE_LOG` 的基本用法。
- 理解 PIE（Play In Editor）是 Editor 内的沙盒世界：PIE 开始时 `UWorld` 被创建，PIE 停止时 `UWorld` 被销毁。

## 必做任务

1. 在 Editor 里把 `AExK1HelloActor` 拖入场景（或在 GameMode::BeginPlay 里 `SpawnActor<AExK1HelloActor>`）。
2. PIE 开始 → 等 2-3 秒 → PIE 停止，在 Output Log 过滤 `LogExK1`，截录完整日志顺序。
3. 实现 `TODO [必做] 1`：在 `BeginPlay` 里打印 `GetWorld() != nullptr` 的结果（应为 `true`）。
4. 实现 `TODO [必做] 2`：在 `EndPlay` 里用 `UEnum::GetValueAsString(EndPlayReason)` 打印触发原因，确认 PIE 停止触发的是 `EndPlayInEditor` 还是 `Destroyed`。
5. 写出完整钩子调用顺序（构造函数 → `PostInitializeComponents` → `BeginPlay` → `Tick`… → `EndPlay` → `Destroyed`），并说明每步时 `GetWorld()` 是否有效。

## 进阶任务

- Override `OnConstruction(const FTransform& Transform)`，在 Editor 中移动 Actor 观察 Construction Script 重新执行。
- 在构造函数里调用 `GetWorld()`，观察 CDO 阶段返回值。
- PIE Players=2 时调用 `GetNetMode()`，记录 Server/Client 两端的 `ENetMode` 值。
- 阅读 `Actor.cpp` 第 4753 行 `AActor::BeginPlay()`，回答：BeginPlay 里对 Component 做了什么，Actor BeginPlay 与 Component BeginPlay 哪个先调用？

## 验收点

- [ ] 编译通过，`.generated.h` 生成到 `Intermediate/Build/.../Inc/ExK1_HelloActor/`
- [ ] Output Log 日志顺序与预期完全一致：构造函数 → `PostInitializeComponents` → `BeginPlay` → `Tick`（多次）→ `EndPlay` → `Destroyed`
- [ ] `EndPlay` 日志里正确打印了 `EndPlayReason` 枚举字符串
- [ ] 能答出四个固定问题

## 观察点

- **构造函数 vs BeginPlay**：构造函数可以 `CreateDefaultSubobject`，但 `GetWorld()` 在 CDO 阶段为 null；`BeginPlay` 时 World 保证有效，是"游戏逻辑真正开始"的时刻。
- **`Tick` 开关**：`PrimaryActorTick.bCanEverTick = true` 必须在构造函数里设置；设为 `false` 时 `Tick` 永远不被调用，可节省性能。
- **`EndPlay` vs `Destroyed` 关系**：`Destroyed()` 内部第一件事是调用 `RouteEndPlay(EEndPlayReason::Destroyed)`，两者有调用栈嵌套关系，不是并列关系。资源清理应放 `EndPlay`，不要只放 `Destroyed`。

## 常见坑

- **忘记 `Super::BeginPlay()`**：不调用 `Super::BeginPlay()` 时，Component 的 `BeginPlay` 不会被调用（见 `Actor.cpp:4764`）。
- **在构造函数里调用 `GetWorld()->SpawnActor`**：CDO 构造期 World 可能为 null 或是 Editor World，会产生非预期行为。
- **`EndPlay` 里访问 `GetWorld()`**：`LevelTransition` 原因时 World 可能正在销毁，某些子系统已不可用。
- **`GENERATED_BODY()` 缺失**：链接报 `AExK1HelloActor::StaticClass` 未定义。

## 提示

- Output Log 过滤框输入 `LogExK1` 可只显示本题日志。
- `UEnum::GetValueAsString` 用法：`UEnum::GetValueAsString(EndPlayReason)` 返回 `FString`。
- Tick 日志设为 `Verbose` 级别，避免刷屏；需在 Log Filter 里将 `LogExK1` 设为 Verbose 才能看到。

## 复盘问题

1. 真正开始执行游戏逻辑的时刻是哪个钩子？构造函数算吗？
2. 谁负责这个 Actor 的生命周期？`DestroyActor` 调用后 Actor 何时被真正回收？
3. `BeginPlay`、`Tick`、`EndPlay`、`Destroyed` 涉及哪些命名线程？
4. Actor 如何对 GC 可见？如果把 Actor 从 `ULevel::Actors` 移除会发生什么？
5. **[本题专属]** 为什么在 Editor 里把 Actor 拖入场景后，构造函数可能被调用多次？CDO 构造与实例化构造有何不同？

## 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h`（`AActor` 类声明，`BeginPlay` / `EndPlay` / `Destroyed` 声明）
- `Engine/Source/Runtime/Engine/Private/Actor.cpp`，第 4753 行 `BeginPlay`，第 3232 行 `EndPlay`，第 3284 行 `Destroyed`，第 3126 行 `PostInitializeComponents`
- `Engine/Source/Runtime/Engine/Classes/Engine/World.h`（`UWorld` 类，`SpawnActor` 声明）
