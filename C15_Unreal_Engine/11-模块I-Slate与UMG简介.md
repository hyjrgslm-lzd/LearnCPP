# 11 模块 I：Slate 与 UMG 简介（可选）

## 模块目标

反向引用 `01-心智模型` 的 §2（Reflection before types）与 §6（Subsystem 化，而非全局单例）。

Slate 是 UE 的 C++ 原生 widget 系统：**无 UObject、无反射、无 GC、全 TSharedPtr 管理**。UMG 是包裹在 Slate 之上的 UObject 反射外壳，让蓝图可以用可视化编辑器组合 UI。这两层各自服务不同的使用场景，但底层共享同一条渲染管线（Slate3D），与主渲染 RDG 管线解耦。

**阶梯定位**：use + inspect

本模块只有 2 道练习，可以在完成模块 D 之后的任意时间插入学习顺序——它不依赖 G/H/F/J 中的任何一个。

---

## Slate vs UMG 关系图

```
┌─────────────────────────────────────────────────────────────────┐
│                        UMG 层（UObject + 反射）                  │
│                                                                   │
│   UWidget ──────────── UPanelWidget ──────────── UUserWidget     │
│   (UCLASS, GC 管理)   (子 widget 容器)            (BP 可扩展)    │
│          │                                               │        │
│          │ RebuildWidget()                  NativeConstruct()     │
│          ▼                                               │        │
│   ┌──────────────────────────────────────────────────────┤        │
│   │         SObjectWidget 桥接层                          │        │
│   │   TSharedPtr<SWidget> ←→ UWidget*                    │        │
│   └─────────────────────────────────────────────────────-┘        │
└─────────────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────────────┐
│                        Slate 层（纯 C++）                         │
│                                                                   │
│   SWidget ──── SCompoundWidget ──── SButton / STextBlock …      │
│   (TSharedPtr 管理，无 UCLASS/UPROPERTY)                         │
│                                                                   │
│   ┌──────────────────────────────────────────────────────────┐   │
│   │   Slate 渲染管线（Slate3D / SlateRHI）                    │   │
│   │   独立于主渲染 RDG；运行在 GameThread 上批量合并绘制       │   │
│   └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

**核心结论**：
- Slate 是 C++ 层无反射 widget 系统，`SWidget` 树完全用 `TSharedPtr` 管理，GC 对它一无所知。
- UMG 在 Slate 上套了一层 `UObject` 反射外壳：每个 `UWidget` 在需要时调用 `RebuildWidget()` 产生对应的 `TSharedRef<SWidget>`，并通过 `SObjectWidget` 这个桥接对象把两者绑定在一起。
- `UUserWidget` 是 UMG 面向蓝图的主入口；`SCompoundWidget` 是自定义 Slate widget 的 C++ 起点。

---

## 正交性说明：为何模块 I 不依赖 G/H/F/J

Slate/UMG 有自己独立的渲染管线（`SlateRHIRenderer`），不经过 RDG（`FRDGBuilder`）。Slate 的绘制在 GameThread 上完成排布（`ArrangeChildren` / `OnPaint`），直接向 `FSlateWindowElementList` 提交绘制元素，由 SlateRHI 批量合并成 GPU 命令。整个路径绕过了 `FSceneRenderer`、`FScene`、`FPrimitiveSceneProxy` 这些 RDG 主渲染路径的核心对象。

因此：
- 不依赖 F（并发/任务图）：Slate widget 的 Tick 和 Paint 都发生在 GameThread，不需要 `ENQUEUE_RENDER_COMMAND`。
- 不依赖 G（RHI/着色器）：不需要手写 `FGlobalShader`，Slate 内部已封装所有 UI 着色器。
- 不依赖 H（RenderGraph）：Slate 不使用 `FRDGBuilder`，没有 transient resource 生命周期问题。
- 不依赖 J（网络/Replication）：UI 状态不通过 NetDriver 同步（UI 绑定数据到 Actor 属性是上层逻辑，不在 Slate/UMG 层）。

唯一的前置依赖是模块 D（UObject/反射/GC），因为 `UUserWidget` 是 `UWidget` 的子类，`UWidget` 是 `UObject` 的子类，使用 `UCLASS`/`UPROPERTY`/`UFUNCTION` 体系。

---

## 模块完成标准

做完本模块，你至少要能稳定说清楚：

1. `SCompoundWidget` 的 `SLATE_BEGIN_ARGS`/`SLATE_END_ARGS` 宏展开产生什么；`SLATE_ARGUMENT`、`SLATE_ATTRIBUTE`、`SLATE_EVENT` 三者的语义差别。
2. `SNew` 和 `SAssignNew` 的区别；Slate widget 为何必须通过 `SNew`/`SAssignNew` 构造而不能用裸 `new`。
3. `ChildSlot` 的作用；如何在 `Construct` 里用 `[ ]` 语法挂布局。
4. `UUserWidget::NativeConstruct`/`NativeTick`/`NativePreConstruct` 三个钩子各在什么时机被调用，与 `AActor::BeginPlay`/`Tick` 的类比。
5. UMG 的 `UWidget::RebuildWidget()` 如何把 UObject 层的描述转化为 Slate 层的 `TSharedRef<SWidget>`；`SObjectWidget` 在其中扮演什么角色。
6. Slate 渲染管线为何与主渲染 RDG 正交；Slate3D 是什么。

---

## 练习 I1 — 最小 SCompoundWidget

### 目标

派生 `SCompoundWidget`，手写完整的 `SLATE_BEGIN_ARGS`/`SLATE_END_ARGS` 宏块、`Construct` 方法与 `ChildSlot` 布局，在 Editor 窗口或 Game Viewport 里显示一个自定义 UI 组件。理解 Slate DSL 的声明式语法是如何通过宏展开实现的，以及 `TAttribute` 惰性求值的实际含义。

### 前置理解

- 你已完成模块 D（`UCLASS`/`UPROPERTY`/GC），理解 UE 的两套生命周期管理。
- 你知道 `TSharedPtr`/`TSharedRef` 的引用计数语义（模块 C）。
- 你接受 Slate widget **不继承 `UObject`**，GC 对其一无所知；它完全由 `TSharedPtr`/`TSharedRef` 的引用计数管理，引用归零时自动析构。
- Slate widget 的构造函数是 `protected` 的——这是刻意设计，强迫所有创建路径都走 `SNew`/`SAssignNew`，确保 widget 在创建时就被包装进 `TSharedRef`，不存在裸指针逃逸。

### 必做任务

1. 在 `C15_Unreal_Engine.uproject` 的某个 Editor Module 里（或新建练习 Module `I1_MinimalSlate`），包含 `SlateCore` 与 `Slate` 依赖（`.Build.cs` 里加入 `"SlateCore"`, `"Slate"`）。

2. 新建头文件 `SMyCounter.h`，派生 `SCompoundWidget`：

   ```cpp
   class SMyCounter : public SCompoundWidget
   {
   public:
       SLATE_BEGIN_ARGS(SMyCounter)
           : _InitialCount(0)
           , _Label(FText::GetEmpty())
           , _OnCountChanged()
       {}
           // SLATE_ARGUMENT：构造时传入一次，不支持惰性绑定
           SLATE_ARGUMENT(int32, InitialCount)

           // SLATE_ATTRIBUTE：支持惰性求值，可以绑定 lambda 或 TAttribute
           SLATE_ATTRIBUTE(FText, Label)

           // SLATE_EVENT：委托回调
           SLATE_EVENT(FSimpleDelegate, OnCountChanged)
       SLATE_END_ARGS()

       void Construct(const FArguments& InArgs);

   private:
       int32 Count = 0;
       TSharedPtr<STextBlock> CountText;
   };
   ```

3. 实现 `SMyCounter.cpp`，在 `Construct` 里用 `ChildSlot` 挂一个 `SVerticalBox` 布局，内含一个 `STextBlock` 显示 Label 和一个 `SButton`。点击按钮时 `Count++` 并刷新 `CountText`。

4. 用 `SNew` 创建 widget 并添加到 Editor 窗口（可用 `FSlateApplication::Get().AddWindow(...)` 或挂到现有 Editor tab），**不**通过 UMG。

5. 验证 widget 确实在屏幕上显示，按钮点击后数字递增。

6. 在代码里写一段注释，回答：为什么 `Construct` 里必须通过 `ChildSlot [ ... ]` 语法挂子 widget，而不是在构造函数里直接赋值？（提示：查看 `SWidget` 的 `SLATE_DECLARE_WIDGET_API` 宏和 `FSlotBase::AttachWidget`。）

### 进阶任务

- 把 `_Label` 改为 `TAttribute<FText>` 绑定一个外部 lambda（例如 `[this]() { return FText::AsNumber(Count); }`），观察 `STextBlock` 是否会在数值变化时自动更新——这就是 `TAttribute` 的**惰性求值**：每次 Slate tick 重绘时都调用绑定的 getter，而不是存一个值副本。

- 在 `Construct` 结束后，用 `ContentScaleAttribute`（`SCompoundWidget` 的成员）设置一个随时间变化的缩放，观察 widget 动画效果。

- 阅读 `Engine/Source/Runtime/SlateCore/Public/Widgets/SWidget.h` 中的 `Tick` 虚函数与 `OnPaint` 虚函数的签名，回答：Slate 的 `Tick` 是在哪条线程、以什么频率被调用的？它与 `AActor::Tick` 有什么本质区别？

- 搜索 `SlateCore/Public/Widgets/DeclarativeSyntaxSupport.h`（或 `Widgets/SlateControlledConstruction.h`），找到 `SLATE_BEGIN_ARGS` 的宏展开定义，读懂它产生了哪些成员函数和 `FArguments` 结构体。

### 验收点

- `SMyCounter` 能在不通过任何 `UWidget`/`UUserWidget` 的情况下显示在屏幕上。
- 能用 `SNew(SMyCounter).InitialCount(5).Label(NSLOCTEXT("I1", "Label", "计数：")).OnCountChanged(...)` 这种链式语法构造 widget。
- `SLATE_ARGUMENT`、`SLATE_ATTRIBUTE`、`SLATE_EVENT` 三者在展开后分别成为 `FArguments` 的什么成员，你能用自己的话描述清楚。
- `TAttribute<FText>` 绑定 lambda 之后，Slate 是在何时（哪个函数里）真正调用 lambda 取值的——你能指出源码路径（提示：`STextBlock::OnPaint` → `TAttribute<T>::Get()`）。
- 四固定问题书面回答完整（见本节末）。

### 观察点

- `SLATE_BEGIN_ARGS(SMyCounter)` 展开后产生一个嵌套结构体 `FArguments`，它继承自 `TSlateBaseNamedArgs<SMyCounter>`。链式调用 `.InitialCount(5)` 是在设置 `FArguments::_InitialCount` 字段，返回 `*this` 以支持继续链接下一个参数——这是 C++ builder 模式在宏中的一次完整实现。
- `SLATE_ARGUMENT` 产生一个值成员 + 一个同名的 setter 方法（返回 `FArguments&`），不支持绑定 lambda。
- `SLATE_ATTRIBUTE` 产生 `TAttribute<T>` 成员 + setter，`TAttribute<T>` 内部可以存一个值，也可以存一个 `TFunction<T()>`（惰性 getter），每次 `Get()` 时才求值。
- `SLATE_EVENT` 产生 delegate 成员 + setter，与 C++ `DECLARE_DELEGATE_*` 系列同语义但更紧凑。
- Slate widget 的生命周期完全由 `TSharedRef`/`TSharedPtr` 的引用计数控制：当所有持有它的 `TSharedPtr` 销毁时，widget 析构，**不需要**也**不应该**把它加入任何 `UPROPERTY` 或 GC 根集。

### 常见坑

- **试图在构造函数里挂子 widget**：`SCompoundWidget` 的构造函数在 `TSharedRef` 完成初始化之前调用，此时 `SharedThis(this)` 还不可用，会触发断言或崩溃。必须在 `Construct(const FArguments&)` 里操作 `ChildSlot`。
- **手动 `delete` SWidget 指针**：Slate widget 通过 `SNew` 返回 `TSharedRef<T>`，全程引用计数管理，永远不要对 Slate widget 调用 `delete`。
- **`SLATE_BEGIN_ARGS` 里漏写默认值初始化**：`FArguments` 的构造函数会调用 `: _Field(DefaultValue)` 初始化列表。如果 `SLATE_ARGUMENT` 没有在 `SLATE_BEGIN_ARGS(...) {}` 的初始化列表里提供合理默认值，后续使用时会读到未初始化数据。
- **把 `SLATE_ATTRIBUTE` 当成 `SLATE_ARGUMENT` 用（或反过来）**：`SLATE_ARGUMENT` 的参数必须是可复制的具体值，不能传 lambda；`SLATE_ATTRIBUTE` 可以传值也可以传 lambda/delegate。类型不匹配会有编译错误，但有时错误信息指向宏展开内部，不直观。
- **在非 GameThread 里修改 Slate widget 状态**：Slate 线程安全模型要求所有 widget 状态修改必须在 GameThread 上进行（Slate tick 也在 GameThread），从 TaskGraph worker 里直接修改 widget 指针会导致竞态。

### 提示

- 找一个最小示例参考：`Engine/Source/Editor/PropertyEditor/Private/SDetailsSplitter.cpp`（或任意 Editor 里的 `SCompoundWidget` 派生类），对照阅读它的 `Construct` 方法和 `ChildSlot` 用法。
- `SVerticalBox`、`SHorizontalBox` 的 slot 用 `+ SVerticalBox::Slot()[ 子widget ]` 语法添加子项，每个 `Slot()` 可以链式配置 `.FillHeight(1.0f)` / `.AutoHeight()` / `.Padding(...)` 等属性。
- `STextBlock` 的 `Text` 是一个 `SLATE_ATTRIBUTE(FText, Text)`，可以直接传 `TAttribute<FText>` 绑定你的惰性 getter。
- 如果不想添加 Editor 窗口的复杂性，可以在 `UUserWidget::NativeConstruct` 里用 `ChildSlot[ SNew(SMyCounter)... ]` 把它嵌入 UMG widget（结合 I2 做）。

### 复盘问题（含四固定问题）

- **Q1（执行真正开始时刻）**：`SMyCounter::Construct` 在哪条线程、哪个时机被调用？是在 `SNew(SMyCounter)` 执行那一刻，还是某个延迟时机？`SNew` 和 `SAssignNew` 调用之后 widget 就已经存在了吗？
- **Q2（生命周期拥有者）**：`TSharedRef<SMyCounter>` 里的 widget 何时销毁？如果你在函数里构造一个 `TSharedRef<SMyCounter>` 但不赋值给任何持久变量，函数返回后发生什么？谁是"拥有者"？
- **Q3（涉及哪些 Named Thread）**：Slate 的 `Tick` 和 `OnPaint` 在哪条 Named Thread 上执行？是 `GameThread`，还是专属的 `SlateThread`？（提示：在单线程 Slate 模式下和多线程 Slate 模式下答案不同；默认 PC 桌面 Editor 使用哪种模式？）
- **Q4（涉及 UObject 时怎么对 GC 可见）**：`SMyCounter` 不是 `UObject`，GC 如何看待它？如果你在某个 `UCLASS` 里用裸 `SMyCounter*` 存了一个指针（而不是 `TSharedPtr<SMyCounter>`），会发生什么？正确的做法是什么？
- **题目特异问题**：`SLATE_ATTRIBUTE` 绑定的 lambda 何时执行？如果你绑定了一个捕获 `this`（某个 UObject）的 lambda，widget 存活期间 UObject 被 GC 回收后会发生什么？如何防御？（提示：用 `TWeakObjectPtr` + 判断 `IsValid`。）
- `ChildSlot [ ... ]` 语法和 `SNew(SVerticalBox) + SVerticalBox::Slot()[ ... ]` 语法在 widget 树结构上有什么区别？
- 为什么 Slate 渲染管线不需要 `ENQUEUE_RENDER_COMMAND`？

### 对应官方参考（源码路径）

- `Engine/Source/Runtime/SlateCore/Public/Widgets/SCompoundWidget.h`：`SCompoundWidget` 定义，`ChildSlot` 成员（`FCompoundWidgetOneChildSlot`），`ContentScaleAttribute` 等 `TSlateAttribute` 示例。
- `Engine/Source/Runtime/Slate/Public/Widgets/Input/SButton.h`：完整 `SLATE_BEGIN_ARGS`/`SLATE_END_ARGS` 宏块示例，`SLATE_ARGUMENT`、`SLATE_ATTRIBUTE`、`SLATE_EVENT` 三者并存。
- `Engine/Source/Runtime/SlateCore/Public/Widgets/SWidget.h`：`SWidget` 基类，`Tick`、`OnPaint`、`ComputeDesiredSize` 虚函数签名；`TSlateAttribute` 的用法。
- `Engine/Source/Runtime/SlateCore/Public/SlotBase.h`：`FSlotBase`，理解 `AttachWidget` 为何必须在 `Construct` 阶段而非构造函数阶段调用。
- `Engine/Source/Runtime/SlateCore/Public/Misc/Attribute.h`（通过 `CoreMinimal.h` 间接引用）：`TAttribute<T>` 的 getter 路径，理解惰性求值机制。

---

## 练习 I2 — UMG + UUserWidget（BP 绑定）

### 目标

派生 `UUserWidget`，在 C++ 里绑定一个 BP 设计器创建的 `UButton` 的 `OnClicked` 委托，用 `NativeConstruct` / `NativeTick` 钩子控制 widget 行为，通过 `AddToViewport` 显示到屏幕。理解 UMG 的 UObject 外壳是如何包裹 Slate 底层的，以及 BP 派生覆盖 C++ 默认行为的机制。

### 前置理解

- 你已完成 I1，理解 Slate 层的 `SCompoundWidget` 和 `TSharedPtr` 生命周期。
- 你已完成模块 D，理解 `UCLASS`/`UPROPERTY`/`UFUNCTION`/GC 机制。
- 你知道 `UUserWidget` 继承自 `UWidget`，`UWidget` 继承自 `UObject`，因此 `UUserWidget` 实例由 GC 管理——和 `SWidget` 完全不同的生命周期路径。
- **关键前置知识（常见坑预警）**：`UUserWidget` 的子组件（通过 `meta=(BindWidget)` 绑定的 `UButton*`、`UTextBlock*` 等）在 **C++ 构造函数执行时还没有被注册和初始化**。在构造函数里访问这些指针会得到 `nullptr` 或未初始化的对象。正确的初始化时机是 `NativeConstruct`（对应 `BeginPlay` 时机）。

### 必做任务

1. 在练习 Module `I2_UMGButton` 里（`.Build.cs` 加入 `"UMG"`, `"SlateCore"`, `"Slate"`），创建 C++ 类 `UMyHUD`，派生自 `UUserWidget`：

   ```cpp
   UCLASS()
   class UI2UMGBUTTON_API UMyHUD : public UUserWidget
   {
       GENERATED_BODY()

   protected:
       // meta=(BindWidget) 告诉 UMG 框架：BP 设计器里必须有一个同名的 UButton
       UPROPERTY(meta=(BindWidget))
       TObjectPtr<UButton> MyButton;

       UPROPERTY(meta=(BindWidget))
       TObjectPtr<UTextBlock> CounterText;

       virtual void NativePreConstruct() override;
       virtual void NativeConstruct() override;
       virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

   private:
       UFUNCTION()
       void OnMyButtonClicked();

       int32 ClickCount = 0;
   };
   ```

2. 实现 `NativePreConstruct`：设置 `CounterText` 的初始文本（**注意**：`NativePreConstruct` 在 Editor 的 Widget Designer 实时预览里也会调用，用于刷新设计器视图，不应在这里做任何有副作用的操作，如绑定委托）。

3. 实现 `NativeConstruct`：在这里绑定 `MyButton->OnClicked`：

   ```cpp
   void UMyHUD::NativeConstruct()
   {
       Super::NativeConstruct();
       if (MyButton)
       {
           MyButton->OnClicked.AddDynamic(this, &UMyHUD::OnMyButtonClicked);
       }
   }
   ```

4. 实现 `OnMyButtonClicked`：`ClickCount++`，用 `FText::AsNumber(ClickCount)` 更新 `CounterText->SetText(...)`.

5. 实现 `NativeTick`：每帧打印一次 `ClickCount` 到屏幕（`GEngine->AddOnScreenDebugMessage`），验证 Tick 确实以帧率运行。

6. 在 Editor 里从 `UMyHUD` 创建 BP 子类 `BP_MyHUD`，在 Widget Designer 里添加同名的 `UButton`（命名为 `MyButton`）和 `UTextBlock`（命名为 `CounterText`）。

7. 在 BP 或 C++ 的 `PlayerController`/`GameMode` 的 `BeginPlay` 里，`CreateWidget<UMyHUD>(...) → AddToViewport()`，运行 PIE，验证 HUD 显示且按钮点击计数正确。

### 进阶任务

- **观察 SObjectWidget 桥接**：在 `UWidget::RebuildWidget` 上打断点（或阅读源码 `Engine/Source/Runtime/UMG/Private/Components/Widget.cpp`），观察 UMG 如何调用派生类的 `RebuildWidget` 产生 `TSharedRef<SWidget>`，再把它包装进 `SObjectWidget`。理解为什么 `SObjectWidget` 需要同时持有 `TWeakObjectPtr<UUserWidget>` 和 `TSharedRef<SWidget>`——前者防止 GC 踢掉 UObject 后 Slate 侧出现悬空回调，后者维持 Slate 树的引用计数。

- **PreConstruct vs Construct 差异**：在 `NativePreConstruct` 和 `NativeConstruct` 里各打一条日志，然后在 Editor Widget Designer 里调整属性，观察 `NativePreConstruct` 是否被触发而 `NativeConstruct` 不被触发。这说明 `NativePreConstruct` 是"设计时预览"的钩子，`NativeConstruct` 是"运行时初始化"的钩子。

- **BP 派生覆盖 C++ 默认行为**：在 `BP_MyHUD` 的蓝图里覆盖 `Construct` 事件，在蓝图里改变 `CounterText` 的颜色。观察：BP 的 `Construct` 会在 C++ 的 `NativeConstruct` 之后调用（因为 `NativeConstruct` 里的 `Super::NativeConstruct()` 会触发 BP 事件链）。这就是"BP 派生覆盖 C++ 默认行为"的机制——BP 并不是替换 C++ 逻辑，而是在 C++ 钩子之后附加执行。

- **`UWidget::RebuildWidget` 的调用时机**：理解 `RebuildWidget` 不是在 `NativeConstruct` 时调用的，而是在 widget 第一次被加入 Slate 树（`TakeWidget()`）时延迟调用的。这意味着一个 `UUserWidget` 可以存在（GC 可见、UPROPERTY 持有），但它的 Slate 子树还不存在——直到 `AddToViewport()` 触发 `TakeWidget()` 才构建 Slate 树。

### 验收点

- `UMyHUD` 的 BP 子类能正确通过 `meta=(BindWidget)` 绑定 `UButton` 和 `UTextBlock`，运行时无 `nullptr` 崩溃。
- 按钮点击后计数器正确递增，`NativeTick` 每帧输出日志。
- 能说清楚为什么 `NativeConstruct` 里可以安全访问 `MyButton`，而构造函数里不能。
- 能说清楚 `UMyHUD` 在 `AddToViewport()` 之前是否存在 Slate 层的 widget 树，以及 Slate 树是何时被构建的。
- 能解释 `SObjectWidget` 在 UMG-Slate 桥接中的角色。
- 四固定问题书面回答完整。

### 观察点

- `UUserWidget` 的生命周期由 GC 管理（因为它是 `UObject` 的子类）：需要有 `UPROPERTY` 持有它，或者在 `AddToViewport()` 之后它会被 viewport 的 widget 树持有（通过 `SObjectWidget`，最终通过 Slate 树的引用链），从而保持 GC 可达——但最安全的做法是在 `APlayerController` 或 `UGameInstanceSubsystem` 里用 `UPROPERTY` 显式持有它。
- `meta=(BindWidget)` 不是普通的 `UPROPERTY`——它是 UMG 框架在 `Initialize()` 阶段通过反射查找同名 widget 来填充这个指针的。如果 BP 设计器里没有对应的同名 widget，UMG 会在编辑器里报错（`Required widget binding 'MyButton' of class UButton was not found`）。
- `AddDynamic` 绑定的函数必须是 `UFUNCTION()`——这是 UMG 的委托使用 `FDynamicMulticastDelegate` 的要求，需要 UHT 生成反射元数据才能通过蓝图桥接调用。Slate 层的 `FOnClicked`（`FReply` 返回类型的 `TDelegate`）和 UMG 层的 `FOnButtonClickedEvent`（`DECLARE_DYNAMIC_MULTICAST_DELEGATE`）是两套不同的委托。
- `NativeTick` 的调用受 `EWidgetTickFrequency` 控制（定义在 `UserWidget.h` L120-131）：默认是 `Auto`，只在有蓝图 tick 函数、latent action、或动画需要播放时才 tick。如果你覆盖了 `NativeTick` 但发现它不被调用，检查 `TickFrequency` 是否被设成了 `Never`，或者类元数据里是否有 `DisableNativeTick`。

### 常见坑

- **在构造函数里访问 `MyButton`（最高频坑）**：`UUserWidget` 通过 `UObject` 构造路径初始化，`meta=(BindWidget)` 的填充发生在 `Initialize()` 阶段（`NativeConstruct` 之前）。构造函数里 `MyButton` 为 `nullptr`，任何访问都是 `nullptr` 解引用。**只在 `NativeConstruct` 及之后访问子组件**。

- **忘记 `Super::NativeConstruct()`**：`NativeConstruct` 里的 `Super::NativeConstruct()` 会触发 `UUserWidget::NativeConstruct` 的默认实现，其中包括触发 BP `Construct` 事件的调用链。漏写 `Super::NativeConstruct()` 会导致 BP 的 `Construct` 事件不触发，BP 侧绑定的逻辑全部失效。

- **`NativeTick` 不被调用**：`UUserWidget` 的 `UCLASS` 声明里有 `meta=(DisableNativeTick)`（见 `UserWidget.h` L282），这是 UMG 的默认设置——UMG 不希望所有 widget 都每帧 tick（性能代价）。如果你需要 `NativeTick`，需要在你的派生类里**移除** `DisableNativeTick` 限制，或者调用 `SetCanTick(true)`。在本练习里，为了验证 tick，可以通过覆盖 `GetDesiredTickFrequency` 返回 `EWidgetTickFrequency::Auto`。

- **`AddToViewport` 之后不持有 `UMyHUD` 指针**：`AddToViewport` 让 viewport 持有 widget 的 Slate 侧引用，但 UObject 侧的生命周期并不因此被 viewport 保证——如果没有 `UPROPERTY` 持有 `UMyHUD*`，GC 有权回收它，回收之后 `SObjectWidget` 里的 `TWeakObjectPtr<UUserWidget>` 会失效，回调也会停止工作。

- **`OnClicked.AddDynamic` 但函数不是 `UFUNCTION`**：`AddDynamic` 是 UMG 动态委托的宏，底层通过函数名字符串注册，需要 UHT 为目标函数生成反射元数据。如果 `OnMyButtonClicked` 漏掉 `UFUNCTION()` 声明，`AddDynamic` 的宏展开会编译通过但运行时无法找到函数，绑定静默失败。

- **在 `NativePreConstruct` 里绑定委托**：`NativePreConstruct` 在 Editor Widget Designer 的 **每次属性变化** 时都会调用，用来刷新 Designer 预览。如果在 `NativePreConstruct` 里 `AddDynamic`，每次预览刷新都会重复添加同一个委托绑定，导致按钮点击触发多次回调。委托绑定必须放在 `NativeConstruct` 里。

### 提示

- 创建 widget 实例的正确 C++ 方式是：
  ```cpp
  // 在 PlayerController::BeginPlay 里
  UMyHUD* HUD = CreateWidget<UMyHUD>(this, UMyHUD::StaticClass());
  // 或者用 BP 子类的 TSubclassOf
  // UMyHUD* HUD = CreateWidget<UMyHUD>(this, HUDClass);
  if (HUD)
  {
      HUD->AddToViewport();
  }
  ```
  不要用 `NewObject<UMyHUD>`——`CreateWidget` 内部做了 widget 特定的初始化（`Initialize()` 调用、Player Context 设置等），`NewObject` 不会触发这些步骤。

- `UButton::OnClicked` 的类型是 `FOnButtonClickedEvent`（`DECLARE_DYNAMIC_MULTICAST_DELEGATE`），而 Slate 层的 `SButton` 里的 `OnClicked` 是 `FOnClicked`（`TDelegate<FReply()>`）。在 UMG 层绑定 `UButton::OnClicked`，不要试图访问 Slate 层的 `SButton::OnClicked`——用 UMG API，不要混层。

- 如果想同时学习 Slate 层的事件绑定，可以在 I1 的 `SMyCounter` 里使用 Slate 的 `FSimpleDelegate` / `FOnClicked`，和 UMG 的 `FOnButtonClickedEvent` 做对比。

### 复盘问题（含四固定问题）

- **Q1（执行真正开始时刻）**：`NativeConstruct` 在引擎启动序列的哪个阶段被调用？是在 `World::BeginPlay` 之前还是之后？`AddToViewport()` 是触发 `NativeConstruct` 的原因吗？（提示：`CreateWidget` 调用 `Initialize` 然后调用 `NativeConstruct`，`AddToViewport` 只是把已经初始化的 widget 挂到 viewport Slate 树。）
- **Q2（生命周期拥有者）**：`UMyHUD` 实例在调用 `AddToViewport()` 之后，谁在保证它不被 GC 踢掉？如果调用 `RemoveFromParent()` 之后，GC 能回收它吗？`UPROPERTY` 持有与"挂在 viewport 上"这两种保活方式有什么区别？
- **Q3（涉及哪些 Named Thread）**：`NativeConstruct`、`NativeTick`、`OnMyButtonClicked` 分别在哪条 Named Thread 上执行？Slate 的输入事件处理（按钮点击）是在 GameThread 上触发的吗？
- **Q4（涉及 UObject 时怎么对 GC 可见）**：`UMyHUD` 里的 `TObjectPtr<UButton> MyButton` 为何加 `UPROPERTY` 而不需要加？（提示：`meta=(BindWidget)` 隐含了 `UPROPERTY`。）如果你在 `UMyHUD` 里持有一个辅助 `UObject*` 子对象但忘记加 `UPROPERTY`，会发生什么？
- **题目特异问题**：`SObjectWidget` 同时持有 `TWeakObjectPtr<UUserWidget>` 和 `TSharedRef<SWidget>`，为什么是 `TWeakObjectPtr` 而不是 `TObjectPtr` 或裸 `UObject*`？如果 UObject 侧的 `UMyHUD` 被 GC 回收，`SObjectWidget` 会如何检测到这一点并停止调用回调？
- `UWidget::RebuildWidget` 和 `UUserWidget::NativeConstruct` 的调用顺序是什么？能否在 `RebuildWidget` 里访问子 widget 组件？
- BP 子类的 `Construct` 事件和 C++ 的 `NativeConstruct` 哪个先执行？如果两者都绑定了同一个 `UButton::OnClicked`，会发生什么？

### 对应官方参考（源码路径）

- `Engine/Source/Runtime/UMG/Public/Blueprint/UserWidget.h` L282 起：`UUserWidget` 的 `UCLASS` 声明，`NativePreConstruct`（L1575）、`NativeConstruct`（L1576）、`NativeTick`（L1578）、`RebuildWidget`（L1568）的声明，以及 `SObjectWidget` 的 `friend class` 声明（L287）。
- `Engine/Source/Runtime/UMG/Public/Components/Widget.h`：`UWidget` 基类，`meta=(BindWidget)` 等 PropertyMetadata 注释（L70-80），`RebuildWidget` 纯虚函数；`SObjectWidget` 引用。
- `Engine/Source/Runtime/Slate/Public/Widgets/Input/SButton.h`：`SButton` 的 `SLATE_BEGIN_ARGS` 里 `SLATE_EVENT(FOnClicked, OnClicked)` — 对照 UMG 层 `UButton::OnClicked`（`FOnButtonClickedEvent`），观察两层委托类型的差异。
- `Engine/Source/Runtime/SlateCore/Public/Widgets/SCompoundWidget.h`：`SCompoundWidget::ChildSlot`（`FCompoundWidgetOneChildSlot`），理解 UMG 的 `UUserWidget::RebuildWidget` 最终产生的 Slate 树根节点是通过什么机制挂上去的。

---

## 做完本模块后你现在应该能说清楚什么

- **Slate 不是 UObject**：`SWidget` 树没有反射、没有 GC 可见性、完全由 `TSharedPtr`/`TSharedRef` 引用计数管理。任何"把 `SWidget*` 存到没有 `TSharedPtr` 保护的地方"都是悬空指针风险。
- **UMG 是 Slate 的 UObject 外壳**：`UWidget::RebuildWidget()` 是把 UObject 描述翻译成 Slate widget 的接口；`SObjectWidget` 是两层的桥接，持有 `TWeakObjectPtr<UUserWidget>` 防止 GC 踢掉 UObject 后 Slate 侧回调悬空。
- **`NativeConstruct` 是 UMG 的 `BeginPlay` 等价物**：不要在构造函数里访问子 widget 组件（它们在 `Initialize()` 之后才填充）；委托绑定要在 `NativeConstruct` 里做；`NativePreConstruct` 是设计时预览钩子，有副作用的操作不放这里。
- **Slate 渲染与主渲染正交**：Slate 有自己的 `SlateRHIRenderer`，走 `FSlateWindowElementList` → 批量合并 → `FRHICommandList`，完全绕开 `FRDGBuilder` / `FScene` / `FPrimitiveSceneProxy`。理解这个解耦让你在调试 UI 性能问题时知道从哪里下手（Slate Stats，不是 GPU Profiler 里的 RDG pass）。
- **`SLATE_ARGUMENT` / `SLATE_ATTRIBUTE` / `SLATE_EVENT` 三者的语义**：`ARGUMENT` 是一次性值，`ATTRIBUTE` 是惰性绑定（每次 `Get()` 才求值），`EVENT` 是委托回调。能在 `SButton.h` 的宏块里一眼辨认出三类用法。

---

## 本模块覆盖的 UE 源码清单

| 文件路径（相对 `Engine/Source/Runtime/`）| 关键内容 |
|---|---|
| `SlateCore/Public/Widgets/SCompoundWidget.h` | `SCompoundWidget` 定义，`ChildSlot`，`TSlateAttribute`，`ContentScaleAttribute` |
| `SlateCore/Public/Widgets/SWidget.h` | `SWidget` 基类，`Tick`/`OnPaint`/`ComputeDesiredSize` 虚函数签名，`TAttribute`/`TSlateAttribute` 用法 |
| `SlateCore/Public/SlotBase.h` | `FSlotBase`，`AttachWidget`，slot 与 parent widget 的关系 |
| `Slate/Public/Widgets/Input/SButton.h` | 完整 `SLATE_BEGIN_ARGS` 宏块示例，`SLATE_ARGUMENT`/`SLATE_ATTRIBUTE`/`SLATE_EVENT` 并存 |
| `UMG/Public/Blueprint/UserWidget.h` | `UUserWidget` 声明，`NativePreConstruct`/`NativeConstruct`/`NativeTick`/`RebuildWidget`，`SObjectWidget` 桥接，`EWidgetTickFrequency` |
| `UMG/Public/Components/Widget.h` | `UWidget` 基类，`meta=(BindWidget)` PropertyMetadata，`RebuildWidget` 纯虚声明 |
