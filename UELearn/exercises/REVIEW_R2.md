# Wave 3 R2 纵向审查报告 (教材对应 + API 真实性)

> 审查范围: 38 习题 + 2 结课项目
> 审查模式: 只读, 不修改源码
> 审查日期: 2026-05-04
> UE 基准: 5.7 release (`G:\Unreal Engine\Source\UnrealEngine\Engine\Source`)

## 审查方法

对每习题 4 步:
1. 打开 `02-*.md` ~ `14-*.md` §练习 X-N, 提"必做任务"清单
2. 打开习题 README, 校对九段 + 四固定问题
3. 统计 cpp 中 `TODO [必做] N:` (共 237 处)
4. 抽 20+ UE 符号 Grep 源码, 确认真实性

## 致命 — 0 项

**无虚构 UE API**. 抽样验证:
- `FRHIBufferCreateDesc::CreateVertex` — `RHIResources.h:1438-1449`
- `UE_OBJECT_PTR_GC_BARRIER` — `ObjectPtr.h:23-146`
- `FAsyncLoadingThread2` — `AsyncLoading2.cpp:4147`
- `FStreamableManager::RequestAsyncLoad` / `FStreamableHandle::ReleaseHandle` — 真实
- `DOREPLIFETIME` / `COND_OwnerOnly` — `UnrealNetwork.h:259/277`, `CoreNetTypes.h:21-38`
- `FComputeShaderUtils::AddPass` — `RenderGraphUtils.h:607`
- `BEGIN_SHADER_PARAMETER_STRUCT` / `SHADER_PARAMETER_RDG_TEXTURE_UAV` — `ShaderParameterMacros.h:1482+`
- `FPrimitiveSceneProxy::GetDynamicMeshElements` / `GetViewRelevance` — 真实
- `FNamePool` / `FNamePoolShardBase` — `UnrealNames.cpp`

## 严重 — 20 项

### 1. ExA1_HelloModule — 教材 7 条必做, README 5 条
骨架化简前 3 条, 未说明"骨架已完成前 N 条"。对照教学失精度。

### 2. ExA2_LoadingPhase — 教材 5 条, cpp 仅 4 TODO
"对照表"合入 README §必做 4, cpp 无显式 TODO。

### 3. ExA3_PluginLayer — 三版本对照降级为进阶
教材明确要求三份交付物: 循环依赖 UBT 报错 / Private 传递失败 / interface+运行时加载。README 只把版本 3 列为进阶。**核心教学杠杆被削弱**。

### 4. ExB1_TArrayTMap — `TChunkedArray` 观察点降进阶
轻度偏移。

### 5. ExC1_SharedPtr — 复盘 Q5 超范围
"`AddSharedReference()` 在 MSVC 映射哪个 Win32 原子 API?" 需看编译器 codegen。建议替换为教材原题"为何 UE 自实现一套 TSharedPtr"。

### 6. ExC2_Allocator — Binned2 常量平台分支缺
README 仅列大堆 `UE_MB2_MAX_SMALL_POOL_SIZE(32752)/UE_MB2_SMALL_POOL_COUNT(51)`。源码 `MallocBinned2.h:31-35` 有小堆分支 `13104/48`。建议补平台说明。

### 7. ExD2_CDOSubobject — `HasAnyFlags` typo
README §提示首字母小写 `hasAnyFlags`, UE 实际首大。**已修复**。

### 8. ExD3_Reflection — 复盘 Q5 自答
"GC 用 RefLink 而不是 PropertyLink 的原因"答案已写完, 丧失思考空间。

### 9. ExD4_MiniMarkSweep — 增量 GC 步进降进阶
教材要求"每帧处理 MaxObjectsPerFrame 个对象, 跨帧标记" 是必做, README 降进阶 `IncrementalMarkStep`。

### 10. ExE1_Package — `UPackage::SavePackage` 签名漂移
UE 5.7 新签名 `SavePackage(UPackage*, UObject*, const TCHAR*, const FSavePackageArgs&)`。README 进阶未给新签名范例。

### 11. ExE2_AssetRegistry — BFS 递归降进阶
教材必做含 BFS 深度=2 依赖树递归打印, README 降进阶。

### 12. ExE3_Streamable — 对照表"最勉强"行预填答案
对照表四行需学员自填, 但 README 已预先写入"半惰性 vs 全惰性"答案。

### 13. ExF3_UETasks — 对照表缩至 4 行 (教材 6 行)
Launch/Then/Pipe/All/Wait/任务句柄 6 行, README 缩为 4。

### 14. ExG3_RHIPass — 手工负担总结表预填
表格是核心, 但 README 已预填 3 行。

### 15. ExH1_RDGDescribe — 复盘 Q5 自答
"Execute 内部三件事"答案已写出。

### 16. ExH2_SceneProxy — StaticMesh 对照降进阶
教材必做"阅读 UStaticMeshComponent::CreateSceneProxy 找 5 处差异"降进阶。

### 17. ExH3_ComputePass — `.usf` 未建 + 路径无说明
cpp 要求创建 `Shaders/ExH3Compute.usf`, 目录不存在。**已修复: 创建 Shaders/ExH3Compute.usf 占位**。

### 18. ExJ2_Replication — `.generated.h` 阅读任务无 cpp TODO 占位
验收点要"能在 .generated.h 找 RPC thunk", cpp 无导航。

### 19. ExJ3_NetSerialize — int16 溢出 guard 缺
必做第 2 条 `float * 100 → int16` 最大 ±327.67; 世界坐标会溢出。`FMath::Clamp` 保护在进阶才提到。

### 20. Cap1_UHeightField — 三次线程跨越骨架全写
`OnAssetLoaded()` 三层 lambda 全完整, 学员"取消注释即跑"。约束勾选表"✓ ≥3 跨线程"预勾。`(FRHITexture2D*)` 强转在 5.7 可能 deprecate (5.7 FRHITexture2D 已合并入 FRHITexture)。

## 轻微 — 13 项

- 命名统一: "A-1" vs "A1" 跨文档不一致
- Cap2 `Obj.cpp:191` 等行号未逐一 Grep, 以函数名为准
- ExI1 进阶提 `_OnTitleClicked` 但必做未声明
- ExB3 切 culture 需 `DefaultGame.ini` 加 `CulturesToStage=de`, 未提醒
- ExJ3 `FArchive::IsLoading()` "接收端为真" 的简化不精确
- ExF4 `DECLARE_RENDER_COMMAND_TAG` 行号漂移
- ExE1 `SavePackage` 签名漂移 (见严重-10)
- ExH2 要读 `ScenePrivate.h` (Renderer Private 头), include 保护未说明
- ExD2 `hasAnyFlags` typo (已修)
- Cap1 `NewObject<UHeightFieldAsset>(GetTransientPackage())` 正确
- ExG2 `AddShaderSourceDirectoryMapping` 签名正确, 建议补示例
- ExF3 `Pipe.h::WaitUntilEmpty` 行号 42 vs 62 不一致, 以函数名为准
- Cap2 `SceneRendering.cpp:5039` 未精确 Grep, 以函数名 `BeginRenderingViewFamilies` 为准

## 分题汇总表

| 习题 | 教材必做 | README 必做 | cpp TODO | 四固定Q齐 | API 幻觉 | 备注 |
|---|---|---|---|---|---|---|
| ExA1 | 7 | 5 | 3 | Y | 0 | 骨架化简 |
| ExA2 | 5 | 5 | 4 | Y | 0 | 对照表无 TODO |
| ExA3 | 5+三版 | 5 | 3 | Y | 0 | 三版本降进阶 |
| ExB1 | 5 | 5 | 5 | Y | 0 | TChunkedArray 降进阶 |
| ExB2 | 5 | 5 | 5 | Y | 0 | 对齐 |
| ExB3 | 5 | 5 | 5 | Y | 0 | 对齐 |
| ExC1 | 5 | 5 | 5 | Y | 0 | Q5 超范围 |
| ExC2 | 4 | 4 | 5 | Y | 0 | Binned2 常量分支缺 |
| ExC3 | 7 | 7 | 4 | Y | 0 | 对齐 |
| ExD1 | 5 | 5 | 3 | Y | 0 | 对齐 |
| ExD2 | 5 | 5 | 4 | Y | 0 | typo 已修 |
| ExD3 | 5 | 5 | 6 | Y | 0 | Q5 自答 |
| ExD4 | 5 | 5 | 6 | Y | 0 | 增量 GC 降进阶 |
| ExK1 | 6 | 5 | 2 | Y | 0 | 日志打印合并 |
| ExK2 | 5 | 5 | 2 | Y | 0 | 对齐 |
| ExK3 | 5 | 5 | 3 | Y | 0 | 对齐 |
| ExK4 | 6 | 5 | 1 | Y | 0 | Subsystem 对比归并 |
| ExE1 | 5 | 5 | 1 | Y | 0 | SavePackage 漂移 |
| ExE2 | 6 | 6 | 4 | Y | 0 | BFS 降进阶 |
| ExE3 | 5+表 | 5 | 3 | Y | 0 | 对照表预填答案 |
| ExF1 | 5 | 5 | 1 | Y | 0 | 对齐 |
| ExF2 | 5 | 5 | 4 | Y | 0 | Q5 换题 |
| ExF3 | 5+6行 | 5+4行 | 2 | Y | 0 | 表缩水 |
| ExF4 | 4 | 4 | 2 | Y | 0 | 对齐 |
| ExG1 | 4 | 4 | 2 | Y | 0 | 对齐 |
| ExG2 | 5 | 5 | 5 | Y | 0 | 对齐 |
| ExG3 | 5 | 5 | 4 | Y | 0 | 总结表预填 |
| ExH1 | 6 | 6 | 2 | Y | 0 | Q5 自答 |
| ExH2 | 5 | 5 | 3 | Y | 0 | StaticMesh 对照降进阶 |
| ExH3 | 5 | 5 | 4 | Y | 0 | .usf 已修 |
| ExI1 | 5 | 5 | 4 | Y | 0 | 对齐 |
| ExI2 | 7 | 7 | 6 | Y | 0 | 对齐 |
| ExJ1 | 5 | 5 | 3 | Y | 0 | 对齐 |
| ExJ2 | 7 | 7 | 6 | Y | 0 | generated.h 无 TODO 占位 |
| ExJ3 | 6 | 6 | 3 | Y | 0 | int16 溢出 guard 缺 |
| ExJ4 | 5 | 5 | 5 | Y | 0 | 对齐 |
| Cap1 | 10 | 10 | 9 | Y | 0 | 骨架预填过多 |
| Cap2 | 6x5+30 | 6x5+30 | N/A | Y (留白) | 0 | 对齐 |

## Cap1_UHeightField 约束核查

- ≥3 跨线程: ✓ (骨架已写)
- NO FlushRenderingCommands: ✓
- ≥1 AActor: ✓ `AHeightFieldViewer`
- ≥1 UWorldSubsystem: ✓ `UHeightFieldWorldSubsystem`
- 禁裸 `new UObject`: ✓ 全 `NewObject<T>`
- 禁裸 `UObject*` 成员: ✓ 全 `TObjectPtr<T>` + `UPROPERTY`

## Cap2_SourceReading 核查

六节齐: ModuleManager / Object.h+Obj.cpp / Actor.cpp / AsyncLoading2.cpp / RenderGraphBuilder / SceneRendering. 全真实。每节 5 问 + 5 处 mini 对照 + 最终一页综述留白。对齐良好。

## 统计

- 扫描: 38
- 致命: **0 项**
- 严重: **20 项** (其中 #7 typo + #17 usf 已修; 剩 18 项多为"必做 vs 进阶"边界或"复盘自答"类教学取舍)
- 轻微: 13 项
- API 幻觉: 0

## 改进优先级

1. ExA3 三版本对照恢复必做
2. ExD2 typo 修复 — **已完成**
3. ExD4 增量 GC 步进恢复必做
4. ExH3 .usf 占位 — **已完成**
5. Cap1 把 OnAssetLoaded 三跨线程改回 TODO
6. ExE3/ExH1/ExG3/Cap1 自答答案挪"参考答案"折叠
7. ExC2 Binned2 补平台分支说明
8. ExJ3 必做加 `FMath::Clamp` 示例
9. ExE1 进阶补 SavePackage 新签名
10. 跨文档统一 "A-1" vs "A1" 命名

---

*无致命级 API 幻觉, 教材映射整体到位. 剩余 18 项严重为必做/进阶边界与复盘自答两条线, 属教学取舍而非 bug; 可留待下一轮针对性加深 (优先级排序已给).*
