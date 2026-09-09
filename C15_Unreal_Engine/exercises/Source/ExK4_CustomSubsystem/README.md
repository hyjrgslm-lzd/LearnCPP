> 对应章节: ../../../06-模块K-Actor-Component-World-Subsystem.md §练习 K-4

## 目标

实现三层 Subsystem（`UEngineSubsystem` / `UGameInstanceSubsystem` / `UWorldSubsystem`），通过多次开始/停止 PIE 观察各层 `Initialize` / `Deinitialize` 调用时机的差异，理解 Subsystem 是"由 UE 框架托管生命周期的单例 UObject"，彻底替代裸全局单例。

## 前置理解

- 已完成 K1-K3，理解 Actor 生命周期与 World 层级。
- 重读 `01-心智模型.md §6（Subsystem 化，而非全局单例）`。
- 知道 `UObject` 的 Outer 链与 GC 可见性。

## 必做任务

1. PIE 启动后过滤 `LogExK4`，观察三层 Subsystem 的 `Initialize` 日志出现顺序。
2. 停止 PIE 后再次启动，确认 `UExK4EngineSubsystem::Initialize` **不再**触发（只在 Editor 启动时调用一次）。
3. 从一个 Actor 的 `BeginPlay` 访问 `UExK4GameInstanceSubsystem`：
   ```cpp
   if (UExK4GameInstanceSubsystem* Sys = GetGameInstance()->GetSubsystem<UExK4GameInstanceSubsystem>())
   {
       Sys->IncrementPlayCount();
   }
   ```
   停止 PIE 后再次启动，确认 `PlayCount` 从 0 重新开始。
4. PIE Players=2 时观察 `UExK4WorldSubsystem::Initialize` 被调用两次（Server World + Client World 各一次）。
5. 填写观察记录表：

   | Subsystem 类型 | Initialize 触发时机 | Deinitialize 触发时机 | PIE 两次时实例数量 |
   |---|---|---|---|
   | `UExK4EngineSubsystem` | | | |
   | `UExK4GameInstanceSubsystem` | | | |
   | `UExK4WorldSubsystem` | | | |

## 进阶任务

- Override `UExK4WorldSubsystem::DoesSupportWorldType`，对 `EWorldType::Editor` 返回 `false`，避免在 Editor World 里创建实例，减少日志噪声。
- 阅读 `Subsystem.h` 中 `ShouldCreateSubsystem(UObject* Outer)` 的默认实现，理解 Subsystem 自动发现机制（反射遍历所有派生 `UCLASS`）。
- Override `UExK4GameInstanceSubsystem::BeginDestroy()`，对比 `Deinitialize` 和 `BeginDestroy` 的调用顺序，理解为何业务清理应放 `Deinitialize`。

## 验收点

- [ ] 编译通过，三层 Subsystem 在 PIE 启动后均打印 Initialize 日志
- [ ] `UExK4EngineSubsystem` 多次 PIE 只 Initialize 一次
- [ ] `UExK4GameInstanceSubsystem::PlayCount` 每次 PIE 从 0 重置
- [ ] PIE Players=2 时 `UExK4WorldSubsystem` 有两个独立实例
- [ ] 能答出四个固定问题

## 观察点

- **Subsystem = 由框架托管的单例 UObject**：不是裸全局，不是 Meyer's singleton——只需继承正确基类，`UCLASS()` + UHT 反射让框架在正确时机自动创建和销毁。
- **GC 可见性**：Subsystem 的 Outer 是宿主容器（GameInstance / World / GEngine），GC 通过 Outer 链可达，无需 `AddToRoot`。
- **`GetSubsystem<T>()` 返回值**：`ShouldCreateSubsystem` 返回 `false` 时返回 `nullptr`，必须做 null check 再使用。

## 常见坑

- **在 `UGameInstanceSubsystem` 里缓存 `UWorld*` 不加 `UPROPERTY`**：GC 看不见该指针，World 销毁后访问是悬空指针。
- **在 `UWorldSubsystem::Initialize` 里期望访问 GameMode/GameState**：此时早于 `OnWorldBeginPlay`，GameMode/GameState 尚未创建。
- **`UEngineSubsystem` 被多次初始化的错觉**：检查是否误放成了 GameInstanceSubsystem 或 WorldSubsystem。

## 提示

- `UWorldSubsystem::DoesSupportWorldType` 默认只对 `EWorldType::Game` 和 `EWorldType::PIE` 返回 `true`，已过滤大多数 Editor World。
- 查看 `Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h` 第 61 行了解 `DoesSupportWorldType` 默认实现。

## 复盘问题

1. Subsystem 的 `Initialize` 在 `BeginPlay` 之前还是之后调用？`OnWorldBeginPlay` 呢？
2. 三层 Subsystem 的 Outer 分别是什么？GC 如何通过 Outer 链保活它们？
3. `Initialize` / `Deinitialize` / `OnWorldBeginPlay` 在哪条命名线程上调用？
4. Subsystem 内部成员若是 `UObject*`，必须加什么才能让 GC 看见？
5. **[本题专属]** CDO 机制和 Subsystem 自动实例化有何关系？`ShouldCreateSubsystem` 是在 CDO 上调用还是在实例上调用？

## 对应官方参考

- `Engine/Source/Runtime/Engine/Public/Subsystems/Subsystem.h`（`USubsystem`，`Initialize` / `Deinitialize` / `ShouldCreateSubsystem`）
- `Engine/Source/Runtime/Engine/Public/Subsystems/EngineSubsystem.h`（`UEngineSubsystem`，`UDynamicSubsystem`）
- `Engine/Source/Runtime/Engine/Public/Subsystems/GameInstanceSubsystem.h`（`UGameInstanceSubsystem`，`Within=GameInstance`）
- `Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h`（`UWorldSubsystem`，`DoesSupportWorldType`，`OnWorldBeginPlay`）
- `Engine/Source/Runtime/Engine/Classes/Engine/GameInstance.h`（`GetSubsystem<T>()`）
