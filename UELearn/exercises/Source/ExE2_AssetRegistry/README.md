> 对应章节: ../../../07-模块E-资产与加载.md §练习 E-2

## 目标

在 Editor 环境中通过 `IAssetRegistry::GetChecked()` 查询资产元数据，理解 `GetAssetsByClass` / `GetDependencies` / `GetReferencers` 三个接口的语义差异；掌握"必须等 `OnFilesLoaded` 后再查询"这一关键时序约束。

## 前置理解

- 知道 `IAssetRegistry` 不加载资产本体，只维护"元数据目录"。
- 知道 Editor 启动时 AssetRegistry 异步扫描 `/Game` 目录，扫描完成前查询结果可能不完整。
- 知道 UE5 中类路径格式迁移到了 `FTopLevelAssetPath`（格式 `/Script/ModuleName.ClassName`）。

## 必做任务

1. 编译模块，Editor 启动后过滤 `LogExE2`，确认 `StartupModule` 日志出现（委托已绑定）。
2. 完成 `TODO [必做] 1`：在 `OnAssetRegistryFilesLoaded` 里调用 `GetAssetsByClass` 查询 `UStaticMesh`，打印 Count 和第一个资产的 `PackageName` / `AssetClassPath`。
3. 完成 `TODO [必做] 2`：打印该资产的全部 `TagsAndValues`（至少 3 个 tag）。
4. 完成 `TODO [必做] 3`：用 `GetDependencies` 查询硬依赖 / 软依赖，分别打印数量与路径。
5. 完成 `TODO [必做] 4`：用 `GetReferencers` 打印前 5 个反向引用（或说明为空的原因）。
6. 写 5-10 行观察记录：硬依赖通常是什么资产？软依赖通常是什么？

## 进阶任务

- 实现 BFS 深度 ≤ 2 的递归依赖树打印，对比一个 `StaticMesh` 的完整依赖树宽度。
- 了解 `FAssetData::TagsAndValues` 的来源：查看 `UStaticMesh::GetAssetRegistryTags` 实现，理解哪些 tag 是在保存时写入的。
- 观察 `IsLoadingAssets()` / `IsGathering()` 在 Editor 启动不同阶段的返回值差异。

## 验收点

- [ ] 编译通过，OnFilesLoaded 回调被正确触发
- [ ] 至少一个真实资产的 `PackageName` / `AssetClassPath` / TagsAndValues（≥ 3 个 tag）已打印
- [ ] `GetDependencies` 硬依赖列表非空（路径以 `/Game/` 或 `/Engine/` 开头）
- [ ] 能解释 `EDependencyProperty::Hard` vs `Soft` 的语义差异
- [ ] 能答出四个固定问题

## 观察点

- **AssetRegistry 不加载资产**：所有查询都不触发磁盘 I/O，只操作内存中的元数据表，毫秒级完成。
- **依赖方向**：`GetDependencies(A)` = "A 依赖了谁"（A→?）；`GetReferencers(A)` = "谁依赖了 A"（?→A）。前者用于"加载 A 需要先加载哪些"，后者用于"删除 A 影响哪些"。
- **硬依赖 vs 软依赖**：硬依赖来自 `TObjectPtr<T>` UPROPERTY 直接引用，Linker 加载时自动一起加载；软依赖来自 `FSoftObjectPath` / `TSoftObjectPtr`，只记录路径，不触发加载。

## 常见坑

- **`StartupModule` 里直接查询返回空结果或崩溃**：AssetRegistry 扫描是异步的，必须等 `OnFilesLoaded`。
- **`IAssetRegistry::Get()` 返回 nullptr**：非常早期加载阶段 AssetRegistry 模块未初始化，用 `IsModuleLoaded("AssetRegistry")` 先判断。
- **`FTopLevelAssetPath` 构造方式**：UE5 中必须用两参数构造 `FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("StaticMesh"))`，不能直接传旧式 `FName` 字符串。
- **`ShutdownModule` 忘记解绑 `FilesLoadedHandle`**：模块卸载后回调野指针，可能在重新加载时 crash。

## 提示

- 如果工程里没有资产，在 Editor 里新建一个空 StaticMesh 并保存，让 AssetRegistry 扫描到，再重启 PIE。
- 打印 TagsAndValues：`AssetData.TagsAndValues.ForEach([](const TPair<FName, FAssetTagValueRef>& Pair){ ... })`。

## 复盘问题

1. `GetAssetsByClass` 的查询发生在哪条线程？AssetRegistry 的后台扫描在哪条线程？两者之间如何加锁？
2. `TArray<FAssetData> AssetList` 是栈上值，但 `TagsAndValues` 指向 AssetRegistry 内部共享数据；如果 AssetRegistry 更新那块数据会发生什么？应如何处理？
3. `OnFilesLoaded` 委托的回调在哪条线程上触发？
4. `IAssetRegistry` 本身是 `UObject`（通过 `UINTERFACE` 定义），其生命周期由谁管理？
5. **[本题专属]** 一个 `UMaterial` 资产对纹理通常是硬依赖还是软依赖？为什么不用软依赖？

## 对应官方参考

- `Engine/Source/Runtime/AssetRegistry/Public/AssetRegistry/IAssetRegistry.h`（`GetAssetsByClass`，`GetDependencies`，`GetReferencers`，`OnFilesLoaded()`）
- `Engine/Source/Runtime/AssetRegistry/Public/AssetRegistry/AssetData.h`（`FAssetData` 字段）
- `UE::AssetRegistry::EDependencyCategory`，`EDependencyProperty`（依赖类型枚举，同 `IAssetRegistry.h`）
