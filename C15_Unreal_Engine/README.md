# C15：Unreal Engine 5.7 引擎架构

## 这套文档要解决什么问题

这不是一套"背 API"的笔记，而是一套"通过系统化练习理解 UE 引擎内核设计"的学习包。

你已经是 C++ 高手——模板、RAII、协程、sender-receiver、ranges 这些不是你的障碍。你真正需要的是：理解 UE 在这些 C++ 基础之上叠加了哪些专属机制，这些机制为什么存在，以及当你阅读或修改引擎代码时如何快速定位、快速推理。

这套文档的目标受众，是已经完成 `C10_Execution` 或 `C06_Ranges`（或同等水平），现在想进入 UE 工程内核层的 C++ 工程师。

**典型的三道撞墙**：

第一道墙是 UBT/UHT。你写了一个完全合法的 C++ 类，但 UE 拒绝编译，因为 UHT（UnrealHeaderTool）必须先跑一遍，产出 `.generated.h`，然后 C++ 编译器才能介入。这不是 C++ 标准里的任何概念。

第二道墙是 GC 与 C++ 所有权并存。你既可以用 `TSharedPtr` 管理非 UObject 的生命周期，也必须用 `UPROPERTY` 告诉 GC 哪些 `UObject*` 指针是可见的。两套体系同时运行，混淆就会产生内存安全问题或 GC 踢掉你以为还活着的对象。

第三道墙是线程模型。GameThread、RenderThread、RHIThread 是 UE 的三条主干线程，它们之间没有自由的数据共享——不用 `ENQUEUE_RENDER_COMMAND` 传值，render thread 侧的代码就会读到垃圾或产生竞态。

这套文档把这三道墙以及更深处的机制，拆成可以一题一题练的形态。

---

## 与 C10_Execution / C06_Ranges 的关系

这是三份正交的姊妹系列：

| 系列 | 核心问题 | 技术层 |
|---|---|---|
| `C06_Ranges` | **如何取值**——惰性值序列的产生与变换 | C++20 `<ranges>`，view、sentinel、CPO、borrowed_range |
| `C10_Execution` | **何时执行**——异步工作图的描述、组合与调度 | P2300 sender-receiver，stdexec，operation_state |
| `C15_Unreal_Engine`（本系列） | **UE 引擎如何把这些组合成一个 editor + runtime**——Module 系统、反射、GC、多线程、渲染、网络 | UE5.7 内核层，UBT/UHT/UObject/RDG/NetDriver |

三份系列覆盖的不是同一层东西。`C06_Ranges` 回答"数据从哪里来"，`C10_Execution` 回答"工作在哪里跑"，`C15_Unreal_Engine` 回答"一个工业级游戏引擎如何把这两件事以及更多的事，组织成一个可以被数百人团队协作的系统"。

学完三份，你对现代 C++ 工程的理解才是完整的：值的组合 + 异步的组合 + 工业级框架的组织方式。

---

## 你会得到什么

### 文档结构（15 份主文档）

- 1 份心智模型总说明（本系列出发点，6 个 mindset shift）
- 10 份模块主文档（A/B/C/D/K/E/F/G/H/I/J），合计 36 道练习题
- 2 份结课项目文档（Cap1 集成项目 + Cap2 源码对照项目）

### 每题你会得到

每道练习题按 6 段式组织：目标 / 前置理解 / 必做任务 / 进阶任务 / 验收点 / 常见坑·观察点·提示。做完每题，你要留下：

1. 可运行代码（每题独立 Module，挂载在 `C15_Unreal_Engine.uproject` 下）
2. 至少一张图（见下文"五张图触发规则"）
3. 5–10 行观察记录
4. 四固定问题书面回答

### 做完全套你应该达到的水平

- 能从零搭建一个可被 UE 加载的 Module，理解 UBT/UHT 的处理顺序
- 能区分 UObject GC 所有权与 C++ `TSharedPtr` 所有权，在两套体系并存时不产生裸悬空指针
- 能在 GameThread 侧安全地向 RenderThread 投递命令，理解值捕获的必要性
- 能读懂一段 RDG pass 代码，并将其映射到 sender-receiver 的"描述期 vs 执行期"框架
- 能解释 Actor / Component / World / Subsystem 四层生态的生命周期钩子与 Outer 链
- 能通过 AssetRegistry 查询资产依赖，用 `FStreamableManager` 异步加载
- 能在 PIE 多人模式下理解 `DOREPLIFETIME` 与 RPC 三向的触发条件
- 能阅读 UE 源码中 5 个以上核心文件，回答"这段代码在哪条线程里跑、谁拥有它的生命周期"

---

## 阅读顺序

按以下顺序阅读，每份文档编号即为推荐阅读顺序：

| 序号 | 文件名 | 主题 | 练习数 |
|---|---|---|---|
| 1 | `01-心智模型.md` | 6 个 mindset shift，无练习 | 0 |
| 2 | `02-模块A-构建系统与模块生命周期.md` | UBT/UHT/`IModuleInterface`/LoadingPhase | 3 |
| 3 | `03-模块B-核心容器与字符串.md` | `TArray`/`TMap`/`FString`/`FName`/`FText` | 3 |
| 4 | `04-模块C-内存-智能指针-委托.md` | `TSharedPtr`/`TWeakPtr`/`TUniquePtr`/Delegate | 3 |
| 5 | `05-模块D-UObject-反射-GC.md` | `UCLASS`/`UPROPERTY`/`NewObject`/GC（枢纽） | 4 |
| 6 | `06-模块K-Actor-Component-World-Subsystem.md` | `AActor`/`UActorComponent`/`UWorld`/Subsystem | 4 |
| 7 | `07-模块E-资产与加载.md` | `UPackage`/`FArchive`/`FStreamableManager` | 3 |
| 8 | `08-模块F-并发与任务图.md` | TaskGraph/`UE::Tasks`/`ENQUEUE_RENDER_COMMAND` | 4 |
| 9 | `09-模块G-RHI与着色器.md` | `FRHICommandList`/`FGlobalShader`/PSO | 3 |
| 10 | `10-模块H-RenderGraph与场景渲染.md` | `FRDGBuilder`/Pass/stdexec 类比（压轴） | 3 |
| 11 | `11-模块I-Slate与UMG简介.md` | `SCompoundWidget`/`UUserWidget`（可选） | 2 |
| 12 | `12-模块J-网络与Replication.md` | `DOREPLIFETIME`/RPC/`NetSerialize`/Prediction | 4 |
| 13 | `13-结课项目1-集成项目.md` | `UHeightField` 串 A–K–H | 1 |
| 14 | `14-结课项目2-源码对照项目.md` | 6 份 UE 源码文件阅读 + 对照表 | 1 |

**依赖顺序即学习顺序**。不建议跳步：每个模块都建立在前序模块的词汇上。唯一的例外是模块 I（Slate/UMG），它正交于渲染主线，可以在完成模块 D 之后的任意时间插入。

---

## 统一技术基线

- **引擎版本**：UE 5.7，release 分支。所有类型、宏、函数名以本机源码 `G:\Unreal Engine\Source\UnrealEngine` 为准。
- **构建工具**：UBT（UnrealBuildTool）+ UHT（UnrealHeaderTool）。**每次修改带有 `UCLASS`/`UPROPERTY`/`UFUNCTION` 宏的头文件后，必须先让 UHT 重新跑一遍，才能进入 C++ 编译阶段。** 直接用编译器处理 `.generated.h` 缺失的文件会报莫名其妙的错误。
- **运行形态**：统一 **Editor-only**。所有练习只要求在 Editor 内跑通，不要求打包、Cook 或 Shipping 发布。这绕开了大量 Cook/Shipping 特有陷阱（`WITH_EDITOR` 守卫、`UCLASS(MinimalAPI)` 可见性、EditorOnly 资产引用等），让你专注于引擎机制本身。
- **网络练习**（模块 J）：统一使用 PIE（Play In Editor）中的"Number of Players ≥ 2"形态，无需独立打包 Server/Client。
- **工程结构**：一练习一独立 Module。每题自己的 `.Build.cs` 声明依赖，宿主 `C15_Unreal_Engine.uproject` 在 `Modules` 数组中统一注册。这种形态让你真实感受依赖裁剪、LoadingPhase 差异、循环依赖打破等 UBT 特有问题。

---

## 四级阶梯定位

学习路径按四级阶梯推进，每个模块在其目标节中明确当前所在阶梯：

| 阶梯 | 含义 | 对应模块 |
|---|---|---|
| **use** | 能正确使用 API，理解参数语义 | A、B、K（首题）|
| **compose** | 能把多个 API 组合成实际工作流 | B、C、E、F（前半）|
| **inspect** | 能读懂实现层，理解设计动机 | A（UHT 生成物）、D（反射遍历）、H（RDG pass）|
| **implement-own** | 能手写关键机制的最小子集 | C（multicast delegate）、D（mini GC）、F（render command）、H（RDG pass）|

四级阶梯不是线性升级，一个模块可能覆盖多级。Module C 从 use 升到 implement-own（手写 delegate），Module D 从 use 经 inspect 到 implement-own（手写 mini mark-sweep），是全套中难度最集中的两个节点。

---

## 每题统一交付物

每道练习完成后，必须留下四样东西：

1. **可运行代码**：`exercises/Xn_<名称>/` 目录下，含 `Private/` 入口文件和 `<Module>.Build.cs`。
2. **至少一张图**（按五张图触发规则，见下节）。
3. **5–10 行观察记录**：不是代码注释，而是你用自己的话写的"我看到了什么、这说明了什么"。
4. **四固定问题书面回答**（见下节）。

如果一道题做完只是"代码跑了"，那还不够。观察记录和四固定问题是把"跑通"变成"理解"的关键步骤。

---

## 每题 6 段式模板

所有练习题按同一模板组织：

```
# Xn — <标题>
## 目标
## 前置理解
## 必做任务
## 进阶任务
## 验收点
## 常见坑 / 观察点 / 提示
```

主文档中每题预填任务描述，用 `// TODO [必做]` 与 `// TODO [进阶]` 在代码骨架中定位填空点。不提供答案文件；用 README 引导式发现。

---

## 四固定问题

每做完一题，必须书面回答以下四个问题：

**Q1：执行真正开始时刻？**
构造期（`.Build.cs` 加载、`IModuleInterface::StartupModule`）？GameThread 的 `BeginPlay`/`Tick`？RenderThread 的命令执行？RHIThread 的 GPU 提交？RDG 的 `Execute` 阶段？定位到具体代码行。

**Q2：生命周期拥有者？**
Module（由 `FModuleManager` 管理）？GC（由 `GUObjectArray` 的 mark-sweep 决定）？`TSharedPtr` 引用计数？`UWorld`（World 销毁时 Actor 销毁）？`UGameInstance`（关卡切换时存活）？RDG transient pool（pass 执行完毕即回收）？

**Q3：涉及哪些 Named Thread？**
`GameThread` / `RenderThread` / `RHIThread` / `NetThread` / `TaskGraph worker pool`。如果有跨线程投递，写出投递点和接收点。

**Q4：涉及 UObject 时怎么对 GC 可见？**
`UPROPERTY` 标记让 GC 可见？用 `TObjectPtr` 替代裸 `UObject*`？`AddToRoot` 强制保活？通过 Outer 链路 reachable（对象挂在 World/Package/Actor 下）？或者这题根本不涉及 UObject（非 UObject 用 `TSharedPtr` 管理）？

四个问题中任意一个答不清，这题就不算真正做完。

---

## 五张图触发规则

不是每题都画所有图，而是随模块触发对应的图类型：

| 图类型 | 触发模块 | 内容描述 |
|---|---|---|
| **模块依赖图** | A（首题必画）、Cap1 | 本 Module 的 `PublicDependencyModuleNames` 节点图，标出 LoadingPhase |
| **线程时序图** | F（全部题）、G、H、Cap1 | 横轴时间，纵轴 GameThread/RenderThread/RHIThread，标出跨线程投递点 |
| **对象生命周期图** | D、K、Cap1 | 从 `NewObject`/`SpawnActor` 到 GC 回收的完整弧，标出 Outer 链 |
| **反射可见性图** | D（反射遍历题）、J（replication 题）| 哪些字段对 UHT 可见，哪些对 GC 可见，哪些对网络 replication 可见 |
| **RDG pass 图** | H（全部题）、Cap1 | pass 节点 + 资源节点 + 依赖边，标出 transient lifetime 范围 |

图可以是文字 ASCII 图、手绘扫描或任意工具产出，关键是它能帮你回答"对象关系是否清楚"，而不只是让代码跑起来。

---

## 建议节奏

### 方案 A：密集 15 天

- 第 1 天：`01-心智模型.md` + 模块 A 全部（3 题）
- 第 2–3 天：模块 B + 模块 C（各 3 题，B 先于 C）
- 第 4–6 天：模块 D（4 题，枢纽，给足时间）
- 第 7 天：模块 K（4 题）
- 第 8–9 天：模块 E + 模块 F 前半（各约 2 题/天）
- 第 10 天：模块 F 后半（含 `ENQUEUE_RENDER_COMMAND`）
- 第 11–12 天：模块 G + 模块 H（渲染主线）
- 第 13 天：结课项目 1（集成）
- 第 14 天：结课项目 2（源码对照）
- 第 15 天：复盘 `01-心智模型.md` + 模块 I（可选）

### 方案 B：标准 4 周

- 第 1 周：01 + A + B + C（基础设施层）
- 第 2 周：D + K（UObject 生态层，此周量最重）
- 第 3 周：E + F + G（资产/并发/RHI 层）
- 第 4 周：H + J + Cap1 + Cap2（压轴 + 结课）

模块 I 穿插在第 3 或第 4 周任意空隙中，不影响主干。

### 方案 C：慢练 9 周

- 第 1 周：01 + A
- 第 2 周：B + C
- 第 3 周：D（单独一周，枢纽模块值得慢练）
- 第 4 周：K（Actor 生态单独消化）
- 第 5 周：E
- 第 6 周：F
- 第 7 周：G + H（渲染主线合并）
- 第 8 周：J + 模块 I（可选）
- 第 9 周：Cap1 + Cap2

慢练方案建议每两个模块做完后回读一遍 `01-心智模型.md`，用当时的理解重新回答那 10 道自测题。

---

## 术语速查表

| 术语 | 所在模块 | 你应该问自己什么 |
|---|---|---|
| `IModuleInterface` | A | 这个接口的 `StartupModule`/`ShutdownModule` 在哪条线程被调用？LoadingPhase 怎么影响调用时机？ |
| `UBT` / `UHT` | A | UHT 在 C++ 编译之前跑，它产出什么文件？如果 `UCLASS` 宏写错了，报错在哪个阶段出现？ |
| `UCLASS` | D | 这个宏展开后 UHT 生成什么？CDO（Class Default Object）是何时创建的？ |
| `UPROPERTY` | D | 不加 `UPROPERTY` 的裸 `UObject*` 成员，GC 能看见它吗？后果是什么？ |
| `UFUNCTION` | D | `UFUNCTION(BlueprintCallable)` 和 `UFUNCTION(Server, Reliable)` 分别让 UHT 生成什么？ |
| `UObject` | D | 所有 `UObject` 都由谁分配？`NewObject<T>` 与 `new T` 的核心区别是什么？ |
| `TObjectPtr<T>` | D | 在 Editor 构建中，`TObjectPtr` 比裸 `T*` 多做了什么（access barrier）？Shipping 中是否相同？ |
| `TSharedPtr<T>` | C | 它管理的对象是 UObject 吗？引用计数线程安全模式 `ESPMode::ThreadSafe` 在哪里必须选？ |
| `FArchive` | E | `FArchive::IsSaving()` 和 `FArchive::IsLoading()` 同一份代码跑两个方向，这是什么序列化模式？ |
| `UPackage` | E | 一个 `.uasset` 文件对应一个 `UPackage`；`UPackage` 里的"顶层 UObject"是谁？ |
| `FRDGBuilder` | H | `AddPass` 时 GPU 命令真的被录制了吗？`Execute` 在哪里被调用？ |
| `FRHICommandList` | G | 它在哪条线程上录制？录制结束后命令什么时候到达 GPU？ |
| `ENQUEUE_RENDER_COMMAND` | F | 这个宏的 lambda 必须按值捕获，为什么？捕获 `UObject*` 裸指针有什么风险？ |
| `AActor` | K | `BeginPlay` 在什么时刻调用（与 `PostInitializeComponents` 的顺序）？`Tick` 在哪条线程？ |
| `UActorComponent` | K | `RegisterComponent` 和 `BeginPlay` 分别发生在什么阶段？ |
| `UWorld` | K | `UWorld` 拥有它所有的 Actor 吗？Level streaming 时 sub-level 的 Actor 归谁拥有？ |
| `UGameInstance` | K | 关卡切换时 `UGameInstance` 存活吗？它比 `UWorld` 的生命周期长多少？ |
| `UWorldSubsystem` | K | 生命周期绑定到哪个对象？PIE 中有几个 `UWorldSubsystem` 实例？ |
| `DOREPLIFETIME` | J | 这个宏必须放在哪个函数里？如果漏写，Replicated 属性会发生什么？ |
| `NetDriver` | J | `UNetDriver` 是每个 World 一个还是全局一个？dedicated server 上有客户端 NetDriver 吗？ |
| `Iris` | J | Iris 是 UE5.5+ 的新 replication 层，与经典 `UNetDriver` 路径并存，入口在 `Runtime/Experimental/Iris/`。 |

---

## UE 特有"破坏纯模板剥皮"点

这套文档处理以下 8 个在纯 C++ 工程中不存在的机制。遇到这些点时，不要用"纯 C++ 直觉"推断行为，要查文档或源码：

**1. UHT 代码生成**（模块 D 首次直面）
`.generated.h` 是 UHT 的产物，不是手写的。它包含反射元数据的注册代码。模块 D 的第一道练习要求你亲手打开 `.generated.h` 读懂其中每一段宏展开的目的。

**2. `SHADER_PARAMETER` 生成**（模块 G）
`BEGIN_SHADER_PARAMETER_STRUCT` / `END_SHADER_PARAMETER_STRUCT` 是第二处代码生成点。它在 C++ 侧和 HLSL 侧同时生效，产生参数绑定代码。模块 G 要求你对照 C++ struct 与 HLSL cbuffer 的字段顺序，理解这层代码生成的工作原理。

**3. Editor-only 守卫**（全模块）
`WITH_EDITOR`、`WITH_EDITORONLY_DATA` 这两个宏控制哪些代码只在 Editor 构建中编译。本套练习统一 Editor-only，所以你不会因为漏写这两个守卫而在 Shipping 构建中报错——但你需要知道它们存在，以及在未来向 Shipping 迁移时哪些地方需要加守卫。

**4. `TObjectPtr` 的 access barrier**（模块 D）
在 Editor 构建中，`TObjectPtr<T>` 不是普通指针的别名——它在解引用时会触发 access tracker，用于检测不通过 GC 系统访问 UObject 的行为。在 Shipping 构建中，`TObjectPtr<T>` 退化为裸指针。模块 D 的进阶任务要求你观察这一差异。

**5. `UPROPERTY` Meta 迷你 DSL**（模块 D、K）
`UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta=(ClampMin="0", ClampMax="100"))` 这里的 `Meta=` 不是 C++ attribute，而是 UHT 识别的特有语法，编译期被解析为反射元数据。不要试图用 C++ 模板机制解释它。

**6. GlobalShader 静态注册**（模块 G）
`IMPLEMENT_GLOBAL_SHADER(FMyShader, "/Plugin/MyShader.usf", "MainCS", SF_Compute)` 通过静态初始化把 shader 注册到全局 shader map。这意味着 shader 不能热重载（不同于普通 C++ 函数），每次改动都需要重新编译 shader。

**7. Render command 值捕获**（模块 F、G）
`ENQUEUE_RENDER_COMMAND` 的 lambda 必须按值捕获所有 UE 资源句柄。原因是 lambda 在 GameThread 构造，但在 RenderThread 异步执行，届时 GameThread 侧的栈变量已经销毁。这是 UE 里最高频的线程安全误用点，也是模块 F 的核心教学内容。

**8. `WITH_EDITOR` / `WITH_EDITORONLY_DATA` 守卫**（全模块）
这两个宏在 Shipping 构建中展开为假，被包裹的代码不编译。如果你把 Editor 工具代码写在类成员里却忘了加 `WITH_EDITORONLY_DATA`，会导致 Shipping 构建的对象布局与 Editor 构建不一致，产生序列化错误或内存损坏。

---

## 一个非常重要的现实提醒

**版本锚定**：本套文档的所有 API 引用、源码路径均以 **UE 5.7 release 分支**为准。UE 各版本间 API 有一定的漂移（尤其是 RDG、`UE::Tasks` 和 Iris 这三个模块在 5.x 系列演化较快）。如果你在 5.5 或 5.6 上做练习，部分 API 签名可能略有不同，以你本地源码为准，设计意图不变。

**UHT/UBT 必须先跑**：每次修改带有 `UCLASS`/`UPROPERTY`/`UFUNCTION`/`USTRUCT`/`UENUM` 宏的头文件后，必须通过 UBT 触发 UHT 重新生成。直接用 IDE 的"仅编译此文件"功能跳过这一步，会得到 `.generated.h` 不匹配的编译错误，和 UHT 语法错误不是同一个层次的报错，容易混淆。

**Editor-only 绕开 Cook/Shipping 陷阱**：本套练习统一不涉及 Cook 或打包。如果你好奇 Cook 行为，可以查阅 `AssetManager` 的 `PrimaryAssetType` 与 Cook filter 机制，但不建议在初学阶段把 Cook 流程引入练习环境——它会把调试成本提高两个量级。

---

## 推荐做题方法

1. 先用一句话写出你认为这题在训练什么。
2. 先回答"四固定问题"的预期答案，再落代码（做完后对照初始预期修正）。
3. 先做"必做任务"，不要一开始就追进阶。
4. 跑通后，不马上进入下一题，先写 5–10 行观察记录。
5. 每做完一个模块，回去重读一次 `01-心智模型.md`。

---

## 最后一句提醒

不要把这套练习当成"我要赶快把 36 道题跑完"。

把它当成两层训练：第一层是框架使用训练——你在学习 UE 把 C++ 工程组织成一个 editor + runtime 的方式，理解它每一个专属机制存在的理由。第二层是框架阅读训练——你在培养看到一段 UE 源码就能快速定位"这在哪条线程、生命周期归谁、GC 能看见什么"的直觉。

两层都练透，你对 UE 引擎内核的理解就不再停留在"能用 API"，而是到达"能推理任意新遇到的引擎代码"。

## C04 泛型与编译期桥接

[进入C04课程](../C04_Generic_CompileTime_Reflection/README.md)。语言静态反射与annotations在C04主讲。本课仍负责UHT生成、UObject运行时信息及GC；语言元信息不会自动替代引擎注册、持久化或对象追踪。

## C05 数据表达先修

字符串编码、标识与显示、基础字段及版本兼容可先读[C05 数据表达](../C05_Data_Representation_Standard_Facilities/README.md)。FString/FName/FText、TCHAR和FArchive的UE具体语义仍按本课固定版本核对，通用Unicode实验不证明所有UE平台布局。
