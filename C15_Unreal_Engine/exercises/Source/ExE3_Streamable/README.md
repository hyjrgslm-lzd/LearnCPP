> 对应章节: ../../../07-模块E-资产与加载.md §练习 E-3

## 目标

用 `FSoftObjectPath` 描述资产地址（不触发加载），通过 `FStreamableManager::RequestAsyncLoad` 发起异步加载请求并获得 `FStreamableHandle`，在 `CompleteDelegate` lambda 里用 `GetLoadedAsset()` 取到加载完毕的对象；建立 `FStreamableHandle` ↔ stdexec `sender` 的精准类比。

## 前置理解

- 已完成 E2，理解 `FAssetData::PackageName` 与 `FSoftObjectPath` 的关系。
- 了解 stdexec sender-receiver 模型：sender 描述工作，`start(op)` 才真正启动，completion 时调用 receiver。
- 知道 `TSoftObjectPtr<T>` 和 `FSoftObjectPath` 的区别：前者是模板包装，后者是通用路径字符串。

## 必做任务

1. 在 Details 面板修改 `AExE3StreamActor` 的 `AssetToLoad` 为工程中真实存在的资产路径（格式：`/Game/PackagePath/AssetName.AssetName`，注意点号）。
2. PIE 启动后过滤 `LogExE3`，确认骨架代码打印出 Handle 返回后的 `HasLoadCompleted()` 状态（应为 0）。
3. 完成 `TODO [必做] 2`：在 CompleteDelegate 里打印 `GetLoadedAsset()->GetName()`，确认是预期资产。
4. 完成 `TODO [必做] 3`：用 `FTimerManager` 或下一 Tick，再次查询 `HasLoadCompleted()`，观察何时变为 `true`。
5. 完成 `TODO [必做] 4`：在 `RequestAsyncLoad` 后立即调用 `StreamableHandle->CancelHandle()`，确认 CompleteDelegate **不被调用**，`WasCanceled()` 返回 `true`。

## 进阶任务

- 改用 `FStreamableManager::AsyncLoadHighPriority`，记录从 BeginPlay 到 CompleteDelegate 的帧数差，对比与 `DefaultAsyncLoadPriority` 的速度。
- 将 `bManageActiveHandle = false`，把 `TSharedPtr<FStreamableHandle>` 存在局部变量里让其超出作用域，观察 Handle 析构后 `GetLoadedAsset()` 的返回值，理解 Handle 生命周期与资产保活的关系。
- 了解 `bStartStalled = true`：创建 Handle 但不立即发起 I/O，直到手动调用 `StartStalledHandle()`，体会"预先描述工作，合适时机批量启动"。

## 验收点

- [ ] 编译通过，PIE 启动后骨架日志正确出现
- [ ] `RequestAsyncLoad` 返回有效 Handle，返回后 `HasLoadCompleted()` 为 `false`
- [ ] CompleteDelegate 在若干帧后被调用，`GetLoadedAsset()` 返回非 null
- [ ] `CancelHandle()` 后 CompleteDelegate 不触发，`WasCanceled()` 为 `true`
- [ ] 复盘记录里包含 StreamableHandle ↔ sender 对照表（见下节）
- [ ] 能答出四个固定问题

## StreamableHandle ↔ sender 对照表（必须填写）

| FStreamableManager / FStreamableHandle | stdexec sender-receiver | 说明 |
|---|---|---|
| `RequestAsyncLoad(Path, Delegate)` 返回 Handle | 构造 sender | I/O 已隐式入队，但资产不在内存中 |
| `GetLoadedAsset()` 在 CompleteDelegate 里调用 | `then` 里取值 / `sync_wait` 后取值 | 只有 completion event 发生后取值才有效 |
| `CancelHandle()` | `stop_token::request_stop()` | 中止加载，阻止 CompleteDelegate 触发 |
| `AsyncLoadHighPriority` | 高优先级 scheduler | 只改变执行顺序，不改变接口形状 |

**最勉强的类比**：`RequestAsyncLoad` 并非"纯惰性"——I/O 已被提交到 AsyncLoading 队列（已隐式 start），而 stdexec sender 构造时"绝不执行任何工作"的保证更严格。这是"半惰性 vs 全惰性"的本质差异。

## 观察点

- **`RequestAsyncLoad` 返回后加载尚未完成**：磁盘读取在 AsyncLoading 专用线程（ALT Thread），真正完成时 `CompleteDelegate` 回到 GameThread 触发。
- **`bManageActiveHandle` 的保活语义**：`true` 时 Manager 内部持有 `TSharedRef`，防止调用方忘记保留 `TSharedPtr` 而导致 Handle 析构；Handle 析构时触发 `ReleaseHandle()`，允许 GC 回收已加载资产。
- **`FStreamableManager` 是 `FGCObject`**：通过重写 `AddReferencedObjects` 手动告知 GC 持有的 UObject，这是非 UObject 对象与 GC 交互的标准方式。

## 常见坑

- **`FSoftObjectPath` 路径格式**：完整格式是 `/Package/Path.AssetName`（注意点号）。只传 `/Package/Path` 不含资产名时，`GetLoadedAsset()` 可能为 null 但 `HasLoadCompleted()` 为 true——静默的路径错误。
- **CompleteDelegate lambda 捕获裸 `this`**：若 Actor 在若干帧后被 `Destroy()`，回调触发时 `this` 是悬空指针。骨架代码已用 `TWeakObjectPtr` 捕获并做 `IsValid()` 检查。
- **`bStartStalled = true` 后必须手动 `StartStalledHandle()`**：否则资产永远不开始加载，日志里没有报错，是静默的"永不完成"陷阱。
- **`TSharedPtr<FStreamableHandle>` 不能放 `UPROPERTY`**：`TSharedPtr` 不是 UObject，GC 不管理它。

## 提示

- 调试时可用 `StreamableHandle->WaitUntilComplete()` 同步等待（阻塞 GameThread），只用于调试，不能用于生产代码。
- `FSoftObjectPath` 路径最安全的来源是 `FAssetData::ToSoftObjectPath()`，避免手写格式出错。

## 复盘问题

1. 磁盘 I/O 在哪条线程的哪个函数里真正开始？`CompleteDelegate` 在哪条线程被调用？
2. 已加载的 `UStaticMesh` 在内存中由谁保活？Handle 被 release 后资产会立即被 GC 回收吗？
3. 画出三点时序：`RequestAsyncLoad`（GameThread）→ 磁盘读取（AsyncLoadingThread）→ `CompleteDelegate`（哪条线程？）
4. `FStreamableManager` 不是 UObject，它通过什么机制让 GC 知道其持有的 UObject 不能回收？
5. **[本题专属]** 对照表四行中哪一行类比最精确？哪一行最勉强？分析 `RequestAsyncLoad` 与 stdexec sender 构造的本质差异。

## 对应官方参考

- `Engine/Source/Runtime/Engine/Classes/Engine/StreamableManager.h`（`FStreamableManager`，`FStreamableHandle`，`RequestAsyncLoad`，`AsyncLoadHighPriority`）
- `Engine/Source/Runtime/CoreUObject/Public/UObject/SoftObjectPath.h`（`FSoftObjectPath`）
- `Engine/Source/Runtime/Engine/Classes/Engine/AssetManager.h`（`UAssetManager::Get()`，`GetStreamableManager()`）
- `Engine/Source/Runtime/CoreUObject/Private/Serialization/AsyncLoading2.cpp`（异步加载底层实现，Cap2 阅读清单第 3 项）
