> 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-3

## 目标

通过运行时代码遍历 `UWorld → ULevel → Actors` 的对象链路，并沿 `GetOuter()` 链向上爬直至 `UPackage`，直观体验 UE 的 Outer 归属体系；理解 `UGameInstance` 在关卡切换时扮演的"跨 World 存活容器"角色。

## 前置理解

- 已完成模块 D 的 `NewObject` 章节，知道 `NewObject<T>(Outer, ...)` 的第一参数决定 Outer。
- 已完成 K1，理解 Actor 生命周期。
- 知道 PIE 模式下有多个 `UWorld` 实例（Editor World / PIE World）。

## 必做任务

1. PIE 启动后在 Output Log 过滤 `LogExK3`，验证骨架代码已打印出当前 Level 名称和 Actor 数量。
2. 完善 `TODO [必做] 2`（已在骨架中提供）：确认 `GetWorld()->GetLevels()` 返回包含 PersistentLevel 的数组。
3. 完善 `TODO [必做] 3`（已在骨架中提供）：观察 Outer 链各层的类名序列（Actor → ULevel → UWorld → UPackage）。
4. 画一张 ASCII 图，标注以下关系的边类型（Outer 链 vs UPROPERTY 引用）：
   ```
   UGameInstance → UWorld → ULevel → AActor → UActorComponent
   ```
5. 阅读 `LevelActor.cpp` 第 671 行，回答：`SpawnActor` 里 `NewObject<AActor>(LevelToSpawnIn, ...)` 的第一参数如何决定新 Actor 的 `GetOuter()` 返回值？

## 进阶任务

- 实现 `TODO [进阶] 1`：在 BeginPlay 里 `SpawnActor<AActor>()`，然后 `check(Spawned->GetOuter() == GetLevel())` 验证 Outer = ULevel。
- 阅读 `FActorSpawnParameters` 结构体（`Actor.h`），找到 `bDeferConstruction` 字段，说明 `SpawnActorDeferred` 模式的用途。
- 比较 PIE 模式与 Standalone 运行时 `UWorld` 创建路径的差异。

## 验收点

- [ ] 编译通过，PIE 启动后 Output Log 出现 Outer 链各层日志
- [ ] Outer 链遍历输出的最后一层是 `UPackage`
- [ ] 能画出 World → Level → Actor → Component 的对象链路图，并标注每条边的类型
- [ ] 能指出 `SpawnActor` 关键步骤在源码的行号
- [ ] 能答出四个固定问题

## 观察点

- **`UWorld` 不是 GC 根**：GC 根集是 `GEngine`（`AddToRoot`），经 `UGameInstance` → `UWorld` 这条链到达 World，World 本身不是根。
- **`ULevel::Actors` 是 `UPROPERTY TArray<AActor*>`**：除 Outer 链之外，这条 UPROPERTY 引用是 GC 扫描 Actor 的第二条路径，双重保险。
- **PIE 中存在多个 UWorld**：Editor World（始终存在）+ PIE World（PIE 期间存在）。在错误 World 里创建的 Actor 会在 PIE 停止时依然存活，是难排查的 Bug 来源。

## 常见坑

- **在非 GameThread 调用 `GetWorld()`**：`AActor::GetWorld()` 假设在 GameThread 调用，跨线程调用产生数据竞争。
- **`GetWorld()` 在 Module `StartupModule` 里为 null**：模块加载阶段早于任何 World 创建。
- **把 `GetCurrentLevel()` 与 `GetLevel()` 混淆**：`GetCurrentLevel()` 返回编辑器当前活动的 Level（不一定等于 Actor 所在 Level）；`GetLevel()` 返回该 Actor 的 Outer Level。

## 提示

- `World->GetLevels()` 返回 `const TArray<ULevel*>&`，包含 PersistentLevel 及所有已加载的 StreamedLevel。
- `GetOutermost()` 直接返回 Outer 链顶端的 `UPackage*`，等效于手动循环 `GetOuter()` 直至 `Cast<UPackage>` 成功。

## 复盘问题

1. `TryAddActorToList` 把 Actor 加入 Level 发生在 `NewObject` 的哪个步骤之后？`PostSpawnInitialize` 又在哪一步？
2. Actor 的生命周期由谁拥有？Outer 链和 UPROPERTY 两条路径哪个先断就会被 GC？
3. `SpawnActor` 全程在哪条命名线程上执行？
4. Actor 通过哪两条路径对 GC 可见？如果只断其中一条会发生什么？
5. **[本题专属]** 为什么跨关卡持久数据应放 `UGameInstanceSubsystem` 而不是 `UWorldSubsystem`？关卡切换时发生了什么？

## 对应官方参考

- `Engine/Source/Runtime/Engine/Private/LevelActor.cpp`，第 455 行 `SpawnActor`，第 671 行 `NewObject<AActor>(LevelToSpawnIn,...)`，第 738 行 `TryAddActorToList`
- `Engine/Source/Runtime/Engine/Classes/Engine/World.h`（`UWorld`，`PersistentLevel`，`StreamedLevels`，`GetLevels()`）
- `Engine/Source/Runtime/Engine/Classes/Engine/Level.h`（`ULevel`，`Actors` 字段）
