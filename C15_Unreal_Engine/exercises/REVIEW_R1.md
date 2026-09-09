# Wave 3 R1 横向审查报告 (Build.cs + 宏语法)

审查范围: 38 个习题目录 (ExA1..ExJ4, Cap1, Cap2)
审查维度: 反向链接 / Build.cs 类名与依赖 / .generated.h 规则 / UCLASS+GENERATED_BODY / IMPLEMENT_MODULE / Module_API 导出宏 / DEFINE_LOG_CATEGORY / TODO 与验证区 / README 反向链接
审查方法: 只读静态扫描, 不编译, 不执行.

注: UE 的 Module_API 宏由 UBT 根据模块目录名自动生成, 规则为目录名全大写, 非字母数字字符转为下划线. 对 ExK1_HelloActor 生成 EXK1_HELLOACTOR_API, 对 ExD4_MiniMarkSweep 生成 EXD4_MINIMARKSWEEP_API. 错拼将导致当前模块函数未找到定义的链接错误 (unresolved external), 或在声明一侧把类变成 __declspec(dllimport) 却没导出, 触发链接失败.

---

## 致命问题 (阻塞编译 / 链接)

### A. Module_API 宏名错拼 (链接时 unresolved external) 10 处

_API 宏由 UBT 根据模块名自动生成. 若在 class 前用错名字, 当前模块编译时该宏无定义 (空), 下游模块/UnrealHeaderTool 生成的 .gen.cpp 里仍按正确名字引用, 造成链接失败或 dllexport 不一致.

- Source/ExD4_MiniMarkSweep/Public/ExD4_MiniMarkSweep.h:55: class EXDBMINIMARKSWEEP_API FMiniGC. 字母 B 错拼 (应为 4). 同文件第 20 行用的是 EXD4_MINIMARKSWEEP_API, 两处自相矛盾. 修复: 改为 EXD4_MINIMARKSWEEP_API.
- Source/ExH2_SceneProxy/Public/ExH2_SceneProxy.h:107: class EXAMPLEH2_API UExH2PrimitiveComponent. 宏名与模块名完全对不上. 修复: 改为 EXH2_SCENEPROXY_API.
- Source/ExE1_Package/Public/ExE1_Package.h:17: class EXEE1_PACKAGE_API UExE1Serializable. 多一个 E. 修复: 改为 EXE1_PACKAGE_API.
- Source/ExE3_Streamable/Public/ExE3_Streamable.h:20: class EXEE3_STREAMABLE_API AExE3StreamActor. 多一个 E. 修复: 改为 EXE3_STREAMABLE_API.
- Source/ExK1_HelloActor/Public/ExK1_HelloActor.h:16: class EXEK1_HELLOACTOR_API AExK1HelloActor. 多一个 E. 修复: 改为 EXK1_HELLOACTOR_API.
- Source/ExK2_ComponentCompose/Public/ExK2_ComponentCompose.h:22: class EXEK2_COMPONENTCOMPOSE_API AExK2Pawn. 多一个 E. 修复: 改为 EXK2_COMPONENTCOMPOSE_API.
- Source/ExK3_WorldLevel/Public/ExK3_WorldLevel.h:18: class EXEK3_WORLDLEVEL_API AExK3Inspector. 多一个 E. 修复: 改为 EXK3_WORLDLEVEL_API.
- Source/ExK4_CustomSubsystem/Public/ExK4_CustomSubsystem.h:15, :29, :52: class EXEK4_CUSTOMSUBSYSTEM_API ... 多一个 E (共 3 处). 修复: 统一改为 EXK4_CUSTOMSUBSYSTEM_API.
- Source/ExJ1_ClientServer/Public/ExJ1_ClientServer.h:54: class EXJ1CLIENTSERVER_API AExJ1ServerSpawned. 缺少模块名中间的下划线. 修复: 改为 EXJ1_CLIENTSERVER_API.
- Source/ExI2_UMGWidget/Public/ExI2_UMGWidget.h:57: class EXI2UMGWIDGET_API UExI2Widget. 缺少下划线. 修复: 改为 EXI2_UMGWIDGET_API.

说明: Cap1_UHeightField 的 4 个头全部正确使用 CAP1_UHEIGHTFIELD_API; ExD2_CDOSubobject, ExD3_Reflection, ExD1_HelloUObject, ExJ2_Replication, ExJ3_NetSerialize, ExJ4_NetPrediction 的 API 宏均正确.

---

## 严重问题 (编译过但行为错 / 命名不一致 / 结构缺失)

### B. README 对应章节格式个别不一致

- Source/ExB2_FString/README.md:1 与 Source/ExB3_FName/README.md:1 : 链接段写作 §练习 B2 / §练习 B3, 其他 B 组 (ExB1) 以及全部其他习题都用带短横的 §练习 B-1/B-2/B-3 格式. 与教材 03-模块B 锚点风格若不一致会点击无法跳转. 建议统一成 §练习 B-2 / B-3.

### C. Build.cs 依赖与 Public 头 include 不完全对齐 (非致命)

- Source/ExI2_UMGWidget/ExI2_UMGWidget.Build.cs : PublicDependency 有 UMG/Slate/SlateCore/Engine/CoreUObject/Core, 但 Public/ExI2_UMGWidget.h:8 已 include Components/TextBlock.h, 第 11 行再对 UButton 做前向声明. Public header 直接 include UMG 子组件头会强制下游拿到 UMG 的传递依赖. 建议: 把 TextBlock.h 移到 .cpp, Public header 只留 class UTextBlock 前向声明, 与第 11 行 UButton 处理一致.
- Source/ExI1_SlateWidget/Public/ExI1_SlateWidget.h:8 include Widgets/SCompoundWidget.h 属于 SlateCore, 与 Build.cs 一致, OK.
- Source/ExG2_GlobalShader/Public/ExG2_GlobalShader.h:9 include RHICommandList.h (RHI 模块), Build.cs 有 RHI, OK.
- Source/ExJ3_NetSerialize/Public/ExJ3_NetSerialize.h : DOREPLIFETIME 实际调用在 .cpp, Public header 未 include Net/UnrealNetwork.h. 不致命, 教学上应在 .cpp 里显式加 include Net/UnrealNetwork.h.

### D. IMPLEMENT_MODULE 第二参数全部正确

所有 37 个练习模块 + Cap1/Cap2 共 38 个 IMPLEMENT_MODULE 的模块名参数与目录名严格一致. IMPLEMENT_PRIMARY_GAME_MODULE 只出现在 Source/UELearn/UELearn.cpp:7, 符合要求. 通过.

### E. DEFINE_LOG_CATEGORY_STATIC 每题独立 category 名

扫描结果: 每个 .cpp 都有自己的 LogExXN / LogCap1Xxx 分类, 未发现重复或与别题串名. 通过.

- Cap1_UHeightField 用了三个子分类 (LogCap1Asset, LogCap1Viewer, LogCap1Subsystem), 没有一个顶层 LogCap1. 按特殊例外条款允许, 但 Cap1_UHeightField.cpp 作为 Module 入口本身未定义任何 category. 建议补一条 LogCap1Module 用于 StartupModule 打印 (非致命, R2 层再处理).

---

## 轻微问题 (风格 / 一致性)

### F. 头文件 include 顺序与 .generated.h 位置

- 全部 17 个带 UCLASS/USTRUCT 的 Public header 均以 include Self.generated.h 结尾 (UHT 硬性要求), 正确.
- Source/ExI2_UMGWidget/Public/ExI2_UMGWidget.h:11 前向声明 class UButton; 紧接其后第 13 行是 include ExI2_UMGWidget.generated.h. UHT 允许前向声明出现在 .generated.h 之前, 当前排版合法.
- 非 UObject 题 (C1/C2/C3/B1/B2/B3/D4/F*/G*/H1/H3/I1/Cap2/ExA1/ExA2/ExA3) 的 Public header 均未含 .generated.h include, 符合纯 C++ 模块不应含 .generated.h 的要求. 通过.

### G. Build.cs 类名与目录名一致

所有 38 个 ModuleRules 派生类的类名均与文件名 (及目录名) 完全一致, PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs; 统一. 通过.

### H. Public 依赖清单合理性

- ExA1: 仅 Core. 合理.
- ExA2: Core + CoreUObject. .cpp 调用 UObject::StaticClass() 故需要 CoreUObject. 合理.
- ExC3: Core + CoreUObject. Public header 声明了 DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam, 需要 CoreUObject. 合理.
- ExC1: 只 Core. 纯 TSharedPtr 演示, 无 UObject. 合理.
- ExG2: Public Core+RenderCore+RHI, Private Renderer+Projects. 分层正确.
- Cap1: Public Core/CoreUObject/Engine, Private RenderCore/RHI/RenderGraph, 渲染细节不泄漏. 合理.

未发现 overlist (硬塞 InputCore/Engine) 或 underlist 致链接失败 (除上述 A 组 API 宏问题外).

### I. TODO 标记与验证区

- 38 个 .cpp 中均扫到 TODO [必做] N: 或 TODO [进阶] N:, 合计 209 处. 通过.
- 38 个 .cpp 中均扫到 UE_LOG / ensureMsgf / 验证区注释块 (合计 409 处). 通过.
- Cap2_SourceReading/Private/Cap2_SourceReading.cpp 仅占位 (按特殊例外), 不含 TODO/验证区. 符合.

### J. 中文注释覆盖率

主要 .cpp/.h 首行对应章节反向链接注释齐全 (扫描 122 个文件全部命中). README 首行反向链接 38 份全部命中. 通过.

### K. 次要命名不一致 (非阻塞)

- Source/ExB2_FString/README.md:1 与 Source/ExB2_FString/ExB2_FString.Build.cs:1 均使用 §练习 B2 (无短横); 其他 B 题一组写法统一成这个亦可. 建议全项目统一 §练习 X-N 带短横, 便于自动抓取锚点.
- Cap2_SourceReading/README.md:3 反向链接放在第 3 行 (前两行是大标题和空行), 格式正确但位置略非第 1 行.
- Source/ExD2_CDOSubobject/Private/ExD2_CDOSubobject.cpp:29 先 IMPLEMENT_MODULE, 第 31 行再 DEFINE_LOG_CATEGORY_STATIC; 其他题普遍是先 DEFINE_LOG 后 IMPLEMENT_MODULE. 两者均合法, 建议统一顺序. 涉及: ExD2, ExD1, ExC3, ExC2, ExC1, ExF4 等.

---

## 额外观察 (正面反馈)

- 所有带 UCLASS/USTRUCT 的头文件均正确把 .generated.h 放在全部其他 include 之后. 这是 UHT 最常见踩坑点.
- GENERATED_BODY() 均放在 class body 第一行, 未发现放在中间.
- IMPLEMENT_PRIMARY_GAME_MODULE 全仓仅在 Source/UELearn/UELearn.cpp 出现一次.
- ExJ3_NetSerialize 正确使用了 template TStructOpsTypeTraits 的 NetSerializer 特化模式.
- ExG2/ExH3 使用 BEGIN_SHADER_PARAMETER_STRUCT 放在 Public header, 不需 .generated.h (此宏是第二处代码生成, 非 UHT), 与 Build.cs 的 RenderCore 依赖匹配.
- Cap1_UHeightField 的 Public/Private 分层严格: 渲染实现只在 PrivateDependency, 不污染下游编译.
- .generated.h 缺失不会影响 C1/C2/D4 等纯 C++ 题 (全部正确未包含).

---

## 统计

- 扫描目录: 38 (全部覆盖)
- 致命 (A 组, _API 宏名错拼, 链接必败): 10 处 / 8 个文件 / 7 个模块
  - ExD4_MiniMarkSweep, ExH2_SceneProxy, ExE1_Package, ExE3_Streamable, ExK1_HelloActor, ExK2_ComponentCompose, ExK3_WorldLevel, ExK4_CustomSubsystem (3), ExJ1_ClientServer, ExI2_UMGWidget
- 严重 (B/C/D/E 组): 3 处
  - B (README 锚点格式不一致): 2 (ExB2, ExB3)
  - C (Public include 含重量级 UMG 头): 1 (ExI2)
  - D/E: 全部通过
- 轻微 (F..K 组): 约 4 处 (IMPLEMENT_MODULE 与 DEFINE_LOG 顺序不统一, 部分跨题风格差异)

建议优先修 A 组 10 处: 在链接期会直接报 unresolved external symbol 或 vtable 缺失. 其他问题可延后到 R2 / 按整体风格 sweep 一并改.
