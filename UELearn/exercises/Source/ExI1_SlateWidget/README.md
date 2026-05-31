> 对应章节: ../../../11-模块I-Slate与UMG简介.md §练习 I-1

## 目标

派生 `SCompoundWidget`，手写完整的 `SLATE_BEGIN_ARGS`/`SLATE_END_ARGS` 宏块、`Construct` 方法与 `ChildSlot` 布局，理解 Slate DSL 的声明式语法是如何通过宏展开实现的，以及 `TAttribute` 惰性求值的实际含义。

## 前置理解

- 已完成模块 D（`UCLASS`/`UPROPERTY`/GC），理解 UE 的两套生命周期管理
- 已完成模块 C（`TSharedPtr`/`TSharedRef` 引用计数语义）
- 接受 Slate widget 不继承 `UObject`，完全由 `TSharedPtr`/`TSharedRef` 引用计数管理
- 理解 Slate widget 的构造函数是 `protected`——强迫所有创建路径走 `SNew`/`SAssignNew`

## 必做任务

1. 完成 `SExI1Widget::Construct` 里的 `ChildSlot` 布局：`SVerticalBox` 内含标题 `STextBlock` + 计数 `STextBlock`
2. 将计数 `STextBlock` 改用 `TAttribute<FText>` 绑定 lambda（`[this]{ return FText::AsNumber(CountValue); }`），观察每帧自动刷新
3. 调用 `SetCounter(N)` 验证 `CountValue` 变化后计数文本更新（TAttribute 惰性路径）
4. 用 `SNew(SExI1Widget).Title(NSLOCTEXT("ExI1", "T", "计数：")).Counter(...)` 链式语法创建 widget
5. 将 widget 显示到屏幕（Editor 窗口 / GameViewport / 嵌入 I2 的 UMG）

## 进阶任务

- 绑定 `_OnTitleClicked` 委托，在 `SButton::OnClicked` 里调用 `Execute()`，演示 `SLATE_EVENT` 的传递
- 阅读 `SlateCore/Public/Widgets/DeclarativeSyntaxSupport.h` 找到 `SLATE_BEGIN_ARGS` 的宏展开定义，读懂它产生了哪些成员函数和 `FArguments` 结构体
- 在 `Construct` 里用 `ContentScaleAttribute` 设置随时间变化的缩放，观察 widget 动画效果

## 验收点

- [ ] `SExI1Widget` 能在不通过任何 `UWidget`/`UUserWidget` 的情况下显示在屏幕上
- [ ] `SLATE_ARGUMENT` / `SLATE_ATTRIBUTE` / `SLATE_EVENT` 三者在展开后分别成为 `FArguments` 的什么成员，能用自己的话描述
- [ ] `TAttribute<FText>` 绑定 lambda 后，Slate 是在何时真正调用 lambda 取值的（指出函数路径）
- [ ] `SetCounter(N)` 调用后计数文本在下次重绘时更新（惰性路径）

## 观察点

- `SLATE_BEGIN_ARGS(SExI1Widget)` 展开后产生 `FArguments` 结构体，链式调用 `.Title(...)` 是在设置 `FArguments::_Title` 字段，返回 `*this` 支持继续链接
- `SLATE_ARGUMENT` 产生值成员 + setter，不支持绑定 lambda
- `SLATE_ATTRIBUTE` 产生 `TAttribute<T>` 成员，可存一个值，也可存 `TFunction<T()>`（惰性 getter）
- Slate widget 生命周期完全由 `TSharedRef` 引用计数控制：无需也不应把它加入任何 `UPROPERTY` 或 GC 根集
- Slate 渲染管线（`SlateRHIRenderer`）与主渲染 RDG 完全正交，无需 `ENQUEUE_RENDER_COMMAND`

## 常见坑

- **在构造函数里挂子 widget**：构造函数调用时 `SharedThis(this)` 不可用，`AttachWidget` 需要 `TSharedRef` 有效；必须在 `Construct(const FArguments&)` 里操作 `ChildSlot`
- **手动 `delete` SWidget 指针**：永远不要对 Slate widget 调用 `delete`，全程引用计数管理
- **`SLATE_BEGIN_ARGS` 里漏写默认值**：`FArguments` 构造函数调用 `: _Field(DefaultValue)` 初始化列表，漏写导致读未初始化数据
- **在非 GameThread 里修改 Slate widget 状态**：Slate tick 和 paint 都在 GameThread，跨线程直接修改 widget 状态导致竞态

## 提示

- 参考 `Engine/Source/Runtime/Slate/Public/Widgets/Input/SButton.h` 里完整的 `SLATE_BEGIN_ARGS` 宏块示例
- `STextBlock` 的 `Text` 是 `SLATE_ATTRIBUTE(FText, Text)`，可直接传 `TAttribute<FText>` 绑定惰性 getter

## 复盘问题（四固定问题 + 本题专属）

1. **执行真正开始时刻？** `SNew(SExI1Widget)` 执行那一刻：内部先 `new SExI1Widget`，立即包装进 `TSharedRef`，再调用 `Construct(FArguments)`——widget 创建是同步的，无延迟
2. **生命周期拥有者？** 最后一个持有 `TSharedPtr<SExI1Widget>` 的变量销毁时 widget 析构；若函数里构造了 widget 但不赋给持久变量，函数返回后引用计数归零，widget 立即析构
3. **涉及哪些 Named Thread？** Slate `Tick` / `OnPaint` 在 **GameThread** 执行（默认 PC 桌面 Editor 单线程 Slate 模式）；多线程 Slate 模式下 `OnPaint` 可在专属 Slate Thread 执行
4. **涉及 UObject 时怎么对 GC 可见？** `SExI1Widget` 不是 UObject，GC 完全不知道它的存在；若在 UCLASS 里用裸 `SExI1Widget*` 存指针（非 TSharedPtr），TSharedRef 析构后变成悬空指针；正确做法是用 `TSharedPtr<SExI1Widget>`
5. **[本题专属]** `SLATE_ATTRIBUTE` 绑定的 lambda 何时执行？答：每次 `TAttribute<T>::Get()` 被调用时，具体路径是 `STextBlock::OnPaint` → `GetText()` → `TAttribute<FText>::Get()` → lambda 求值

## 对应官方参考

- `Engine/Source/Runtime/SlateCore/Public/Widgets/SCompoundWidget.h`（`SCompoundWidget` 定义，`ChildSlot` 成员）
- `Engine/Source/Runtime/Slate/Public/Widgets/Text/STextBlock.h`（`STextBlock` 完整宏块示例）
- `Engine/Source/Runtime/Slate/Public/Widgets/Input/SButton.h`（`SLATE_ARGUMENT`/`SLATE_ATTRIBUTE`/`SLATE_EVENT` 并存示例）
- `Engine/Source/Runtime/SlateCore/Public/Misc/Attribute.h`（`TAttribute<T>` 惰性 getter 路径）
