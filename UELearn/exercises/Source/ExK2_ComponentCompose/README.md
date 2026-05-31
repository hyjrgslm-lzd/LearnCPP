> 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-2

## 目标

掌握 `USceneComponent` 的 transform 附着层级，理解 `CreateDefaultSubobject`（构造期）与运行时 `NewObject + RegisterComponent` 的本质差异；能在运行时正确动态添加 Component 并验证其注册状态。

## 前置理解

- 已完成 K1，理解 Actor 构造函数与 `BeginPlay` 的区别。
- 知道 `UPROPERTY` 对 GC 的作用（Component 成员必须加 `UPROPERTY` 才能保证 GC 可见）。
- 了解 `FTransform`（位置/旋转/缩放）的基本概念。

## 必做任务

1. 编译 `AExK2Pawn`，PIE 启动后确认 Root/Mesh 已创建（Output Log 有构造函数日志）。
2. 实现 `TODO [必做] 1`：在 `BeginPlay` 里把 `Mesh` 的相对位置设为 `(100, 0, 0)`，打印 `Mesh->GetComponentLocation()`（世界坐标应为 Root 位置 + 偏移）。
3. 实现 `TODO [必做] 2`：运行时动态添加 `DynamicChild`，调用 `RegisterComponent()`，打印 `DynamicChild->IsRegistered()`（应为 `true`）。
4. 说明 `SetupAttachment`（构造期）与 `AttachToComponent`（运行期）的区别：前者只设置 `AttachParent` 指针，不触发 transform 更新；后者执行完整 transform 接管。
5. 尝试在代码里对 `UActorComponent`（非 SceneComponent）调用 `GetComponentLocation()`，观察编译错误，理解"SceneComponent = ActorComponent + Transform"的设计分层。

## 进阶任务

- 阅读 `ActorComponent.cpp` 第 1923 行 `RegisterComponentWithWorld()`，理解注册时做了哪几件事（设置 World 指针、触发 `OnRegister` 回调、通知 Owner）。
- 在 `EndPlay` 里调用 `DynamicChild->DestroyComponent()`，override `OnUnregister()` 并打印日志，观察卸载时机。
- 对比 `SetupAttachment` 与 `AttachToComponent` 在已注册 Component 上的行为差异。

## 验收点

- [ ] 编译通过，`Root` / `Mesh` 在 Details 面板可见
- [ ] `DynamicChild->IsRegistered()` 日志输出 `true`
- [ ] 理解为何 `CreateDefaultSubobject` 只能在构造函数里调用（引擎有 `check` 断言保护）
- [ ] 能解释 `UPROPERTY` 缺失时 Component 成员面临的 GC 风险
- [ ] 能答出四个固定问题

## 观察点

- **`CreateDefaultSubobject` 的特殊性**：只能在构造函数（包括 `FObjectInitializer` 回调）里调用，产生以 Actor 为 Outer、被 GC 跟踪的子对象。运行期调用会触发 `check` 失败。
- **运行时添加 Component 的必要步骤**：`NewObject`（创建，Outer = Actor）→ `SetupAttachment` 或 `AttachToComponent`（设置层级）→ `RegisterComponent()`（注册到 World，启用 Tick/Render）。
- **非 SceneComponent 不参与 transform 层级**：`UActorComponent` 子类没有世界坐标，不挂在 SceneComponent 的父子树上。

## 常见坑

- **运行期添加 Component 忘记 `RegisterComponent()`**：`NewObject` 只创建内存对象，Component 的 `BeginPlay` 和 `Tick` 都不会被调用。
- **`TObjectPtr<USceneComponent>` 成员不加 `UPROPERTY`**：GC 可能在下次运行时回收该 Component，产生悬空指针。
- **运行期用 `SetupAttachment` 而不是 `AttachToComponent`**：`SetupAttachment` 在运行期能编译，但不更新 transform 继承，子 Component 位置不跟随 Parent。
- **Component Outer 不是 Actor**：`RegisterComponent` 内部 `check(GetOwner() != nullptr)`，Outer 不是 Actor 会 crash。

## 提示

- `GetActorLocation()` 内部直接调用 `RootComponent->GetComponentLocation()`，验证"Actor transform 由 RootComponent 代理"。
- 查看 `SceneComponent.h` 中 `SetupAttachment` 的注释："This is designed to be called only in the constructor"。

## 复盘问题

1. `RegisterComponent()` 调用后，Component 的 `BeginPlay` 在什么时机被调用？
2. `CreateDefaultSubobject` 产生的 Component 与 `NewObject` 产生的 Component 在 GC 可见性上有何差异？
3. `RegisterComponent` 在哪条命名线程上调用？`OnRegister` 回调在哪条线程？
4. Component 如何对 GC 可见？Outer 链 vs `UPROPERTY` 两条路径各自的保证是什么？
5. **[本题专属]** 为什么 `AttachToComponent` 在构造期调用"能编译但语义错误"？它会在构造期做哪些额外操作导致问题？

## 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/Components/ActorComponent.h`（`UActorComponent` 基类，`RegisterComponent`）
- `Engine/Source/Runtime/Engine/Classes/Components/SceneComponent.h`（`SetupAttachment` / `AttachToComponent` 声明）
- `Engine/Source/Runtime/Engine/Private/Components/ActorComponent.cpp`，第 1542 行 `OnRegister`，第 1923 行 `RegisterComponentWithWorld`
