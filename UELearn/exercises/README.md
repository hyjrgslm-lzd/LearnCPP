# UELearn 习题集

> 对应教材: [../README.md](../README.md) (P:/C++Code/UELearn 根目录)
>
> 风格参考: [P:/C++Code/Ranges_Study/exercises](../../Ranges_Study/exercises), [P:/C++Code/Execution_Study/exercises](../../Execution_Study/exercises)

本目录是 UELearn 教材的**可动手配套习题**。每题是一个独立 UE 游戏 runtime module, 以 `ExXN_Topic/` 命名 (X = 模块字母, N = 题号)。共 **36 习题 + 2 结课项目 = 38 独立模块**。

---

## 定位

教材 (`P:/C++Code/UELearn/*.md`) 专注**文字讲义 + 内嵌片段**; 本目录专注**可编译骨架 + TODO 填空 + 验证观察点**。

两者通过文件头 `// 对应章节: ../../../NN-xxx.md §练习 X-N` 双向链接。

---

## 如何使用

1. 详细打开 / 编译 / PIE 流程见 [BUILD_GUIDE.md](./BUILD_GUIDE.md)
2. **强烈建议**按下表顺序做题 (A→B→C→D→K→E→F→G→H→I→J→Cap), 每做一题:
   - 读 `ExXN_Topic/README.md` 九段 (目标 / 前置 / 必做 / 进阶 / 验收 / 观察 / 常见坑 / 提示 / 复盘)
   - 读教材对应章节
   - 填 `.cpp` / `.h` 里的 `// TODO [必做]` 与可选 `// TODO [进阶]`
   - 编译 → PIE → 查看 `LogEx*` 分类日志 → 对照「预期输出」
   - 完成后解开 cpp 末尾「验证区」注释块, 跑 `ensureMsgf` 断言
   - 写下 README 九段里「复盘问题」的答案

---

## 习题索引表

> 顶层 README 在 Wave 4 回填全表; 目前为占位, 见各题 README 与教材大纲 `00-课程大纲.md`.

| ID | 目录 | 教材章节 | 层级 | 关键 API |
|---|---|---|---|---|
| A1 | ExA1_HelloModule | 02-模块A §A-1 | use | `IMPLEMENT_MODULE` |
| A2 | ExA2_LoadingPhase | 02-模块A §A-2 | use | `ELoadingPhase` |
| A3 | ExA3_PluginLayer | 02-模块A §A-3 | inspect | `IPlugin` / `.uplugin` |
| B1 | ExB1_TArrayTMap | 03-模块B §B-1 | use | `TArray` / `TMap` / `TSet` |
| B2 | ExB2_FString | 03-模块B §B-2 | use | `FString` |
| B3 | ExB3_FName | 03-模块B §B-3 | inspect | `FName` / `FText` |
| C1 | ExC1_SharedPtr | 04-模块C §C-1 | use | `TSharedPtr` |
| C2 | ExC2_Allocator | 04-模块C §C-2 | inspect | `FMemory` / LLM |
| C3 | ExC3_Delegate | 04-模块C §C-3 | implement-own | 手写 multicast delegate |
| D1 | ExD1_HelloUObject | 05-模块D §D-1 | use | `UCLASS` / `.generated.h` |
| D2 | ExD2_CDOSubobject | 05-模块D §D-2 | inspect | `CreateDefaultSubobject` / CDO |
| D3 | ExD3_Reflection | 05-模块D §D-3 | inspect | `TFieldIterator<FProperty>` |
| D4 | ExD4_MiniMarkSweep | 05-模块D §D-4 | implement-own | 手写 mark-sweep |
| K1 | ExK1_HelloActor | 06-模块K §K-1 | use | `AActor` 五时机 |
| K2 | ExK2_ComponentCompose | 06-模块K §K-2 | use | `UActorComponent` 组合 |
| K3 | ExK3_WorldLevel | 06-模块K §K-3 | inspect | `UWorld` / `ULevel` |
| K4 | ExK4_CustomSubsystem | 06-模块K §K-4 | use | `UWorldSubsystem` |
| E1 | ExE1_Package | 07-模块E §E-1 | inspect | `UPackage` / `FArchive` |
| E2 | ExE2_AssetRegistry | 07-模块E §E-2 | use | `IAssetRegistry` |
| E3 | ExE3_Streamable | 07-模块E §E-3 | use | `FStreamableManager` |
| F1 | ExF1_Runnable | 08-模块F §F-1 | use | `FRunnable` / `FEvent` |
| F2 | ExF2_TaskGraph | 08-模块F §F-2 | use | `FFunctionGraphTask` |
| F3 | ExF3_UETasks | 08-模块F §F-3 | use | `UE::Tasks::Launch` |
| F4 | ExF4_RenderCmd | 08-模块F §F-4 | implement-own | `ENQUEUE_RENDER_COMMAND` 值捕获 |
| G1 | ExG1_RHIResource | 09-模块G §G-1 | inspect | `FRHICommandListImmediate` |
| G2 | ExG2_GlobalShader | 09-模块G §G-2 | inspect | `IMPLEMENT_GLOBAL_SHADER` |
| G3 | ExG3_RHIPass | 09-模块G §G-3 | inspect | 手动 RHI Draw Pass |
| H1 | ExH1_RDGDescribe | 10-模块H §H-1 | inspect | `FRDGBuilder` 描述期 |
| H2 | ExH2_SceneProxy | 10-模块H §H-2 | inspect | `FPrimitiveSceneProxy` |
| H3 | ExH3_ComputePass | 10-模块H §H-3 | implement-own | 手写 RDG Compute Pass |
| I1 | ExI1_SlateWidget | 11-模块I §I-1 | use | `SCompoundWidget` |
| I2 | ExI2_UMGWidget | 11-模块I §I-2 | use | `UUserWidget` |
| J1 | ExJ1_ClientServer | 12-模块J §J-1 | use | `UNetDriver` / PIE 多客户端 |
| J2 | ExJ2_Replication | 12-模块J §J-2 | use | `DOREPLIFETIME` |
| J3 | ExJ3_NetSerialize | 12-模块J §J-3 | implement-own | `NetSerialize` / `FFastArraySerializer` |
| J4 | ExJ4_NetPrediction | 12-模块J §J-4 | inspect | 客户端预测 + 校正 |
| Cap1 | Cap1_UHeightField | 13-结课项目1 | integrate | A→K→H 全链 |
| Cap2 | Cap2_SourceReading | 14-结课项目2 | integrate | 6 文件源码对照 |

---

## 四个固定问题

每题 README 的「复盘问题」必含:

1. **真正开始执行的时刻?** (timing / thread / phase)
2. **谁负责这对象的生命周期?** (ownership)
3. **涉及哪些命名线程?** (GameThread / RenderingThread / RHIThread / TaskGraph pool)
4. **这对 GC 如何可见?** (UPROPERTY / TObjectPtr / AddToRoot / 不可见)

---

## 验证不含自动化测试

沿袭 Ranges/Execution 风格, 本习题集**不含 gtest / automation test framework**, 验证靠:

- 编译通过 (语法层)
- `UE_LOG(LogEx*, ...)` 输出对照 README 「预期输出」
- `ensureMsgf` 断言 (完成 TODO 后解开注释)
- 填答「复盘问题」

---

## 质量保证

本目录经 Wave 3 双 reviewer 独立审过:
- **R1 横向** (Build.cs + UE 宏语法): 报告 [REVIEW_R1.md](./REVIEW_R1.md)
- **R2 纵向** (教材对应 + API 真实性): 报告 [REVIEW_R2.md](./REVIEW_R2.md)

### 已修复
- 10 处 `*_API` 导出宏名错别字 (ExD4/ExH2/ExE1/ExE3/ExK1/ExK2/ExK3/ExK4/ExJ1/ExI2) — 阻塞 link 级别
- ExD2 `hasAnyFlags` typo → `HasAnyFlags`
- ExH3 `Shaders/ExH3Compute.usf` 占位文件

### 待学员自行完善 (R2 标记"严重-边界取舍" 18 项)
多数为"必做 vs 进阶"边界把握与"复盘问题是否自答", 属教学风格取舍非 bug:
- ExA3 三版本对照恢复必做 (课程核心杠杆)
- ExC2 Binned2 常量补平台分支 (13104/48 小堆与 32752/51 大堆)
- ExD4 增量 GC 步进恢复必做
- ExE1 进阶 `UPackage::SavePackage` 5.7 新签名
- ExE3/ExH1/ExG3/Cap1 对照表预填答案可挪至折叠"参考答案"
- ExJ3 必做加 `FMath::Clamp` 保护量化溢出
- Cap1 三线程跨越骨架已完整, 学员动手空间偏小

### API 幻觉: 0 项
R2 抽样 20+ UE 符号 Grep `G:/Unreal Engine/Source/UnrealEngine/Engine/Source` 全部命中, 签名一致。

## 改动历史

- 2026-05-04 生成本 exercises 目录 (38 模块骨架, 双 reviewer 审过, 10 处 link-阻塞错别字已修)
