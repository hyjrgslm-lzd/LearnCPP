> 对应章节: ../../../11-模块I-Slate与UMG简介.md §练习 I-2

## 目标

派生 `UUserWidget`，在 C++ 里用 `meta=(BindWidget)` 绑定 BP 设计器创建的 `UTextBlock` 和 `UButton`，通过 `NativeConstruct` 钩子绑定委托，实现点击按钮更新文本的完整 UMG 工作流。

## 前置理解

- 已完成 I1（Slate 层 `SCompoundWidget` 和 `TSharedPtr` 生命周期）
- 已完成模块 D（`UCLASS`/`UPROPERTY`/`UFUNCTION`/GC 机制）
- 知道 `UUserWidget` 继承自 `UWidget`，`UWidget` 继承自 `UObject`，由 GC 管理（与 SWidget 完全不同）
- **高频坑预警**：`UUserWidget` 的 `meta=(BindWidget)` 子组件在构造函数里是 `nullptr`，只在 `NativeConstruct` 及之后安全访问

## 必做任务

1. 在 Editor 里从 `UExI2Widget` 创建 BP 子类 `BP_ExI2Widget`
2. 在 Widget Designer 里添加 `UTextBlock` 命名为 `TitleLabel`（名称必须与 C++ 成员变量名一致）
3. 完成 `NativeConstruct` 里的 `TitleLabel` 初始化（调用 `SetText`）
4. 在 .h 中取消 `ConfirmButton` 声明注释，在 Widget Designer 里添加同名 `UButton`
5. 在 `NativeConstruct` 里用 `AddDynamic` 绑定 `OnConfirmClicked`
6. 实现 `OnConfirmClicked`：每次点击更新 `TitleLabel` 文本（显示点击次数）
7. 在 `PlayerController::BeginPlay` 里 `CreateWidget<UExI2Widget>(this, ...) → AddToViewport()`，PIE 验证

## 进阶任务

- 在调试器里给 `UWidget::RebuildWidget` 打断点，观察 Slate 树构建时机（`AddToViewport()` 触发，而非 `CreateWidget` 时）
- 在 `NativePreConstruct` 和 `NativeConstruct` 里各打一条日志，在 Editor Widget Designer 里调整属性观察哪个被触发
- 在 `BP_ExI2Widget` 的蓝图里覆盖 `Construct` 事件，改变 `TitleLabel` 颜色，观察 BP Construct 在 C++ `NativeConstruct` 之后执行

## 验收点

- [ ] `UExI2Widget` 的 BP 子类通过 `meta=(BindWidget)` 绑定 `TitleLabel` 和 `ConfirmButton`，运行时无 nullptr 崩溃
- [ ] 按钮点击后计数器正确递增并更新 `TitleLabel`
- [ ] 能说清楚为什么 `NativeConstruct` 里可以安全访问 `TitleLabel`，而构造函数里不能
- [ ] 能说清楚 `AddToViewport()` 之前是否存在 Slate 层的 widget 树，以及 Slate 树何时被构建

## 观察点

- `meta=(BindWidget)` 的填充发生在 `UUserWidget::Initialize()` 阶段（晚于 UObject 构造函数）；`NativeConstruct` 在 `Initialize()` 末尾触发
- `AddToViewport()` 让 viewport 持有 widget 的 Slate 侧引用，但 **UObject 侧生命周期不由 viewport 保证**——若没有 `UPROPERTY` 持有 `UExI2Widget*`，GC 有权回收
- `NativeTick` 默认不被调用（`meta=(DisableNativeTick)`）；若需要每帧 tick，覆盖 `GetDesiredTickFrequency()` 返回 `EWidgetTickFrequency::Auto`
- BP 子类的 `Construct` 事件在 C++ `NativeConstruct` 之后执行（Super 调用触发 BP 链）

## 常见坑

- **在构造函数里访问 `TitleLabel`（最高频坑）**：`BindWidget` 填充在 `Initialize()`，构造函数里永远是 `nullptr`
- **忘记 `Super::NativeConstruct()`**：漏写导致 BP 的 `Construct` 事件不触发，BP 侧绑定的逻辑全部失效
- **`AddDynamic` 但函数不是 `UFUNCTION()`**：编译通过但运行时绑定静默失败
- **`AddToViewport` 后不持有 `UExI2Widget*`**：GC 回收 UObject 后 `SObjectWidget` 里的 `TWeakObjectPtr` 失效，回调停止
- **在 `NativePreConstruct` 里绑定委托**：每次 Editor 预览刷新都重复添加，导致点击触发多次回调

## 提示

- 创建 widget 实例：`CreateWidget<UExI2Widget>(this, UExI2Widget::StaticClass())`，不要用 `NewObject<UExI2Widget>`
- `UButton::OnClicked` 是 `FOnButtonClickedEvent`（`DECLARE_DYNAMIC_MULTICAST_DELEGATE`）；Slate 层的 `SButton::OnClicked` 是另一套委托，不要混用两层

## 复盘问题（四固定问题 + 本题专属）

1. **执行真正开始时刻？** `NativeConstruct` 在 `CreateWidget` 内部（`Initialize()` 末尾）调用，不是 `AddToViewport()` 触发；`AddToViewport()` 只是把已初始化的 widget 挂到 viewport Slate 树
2. **生命周期拥有者？** GC（通过 `UPROPERTY` 持有或 viewport Slate 树间接保活）；`RemoveFromParent()` 后若无 `UPROPERTY` 持有，GC 可回收
3. **涉及哪些 Named Thread？** `NativeConstruct` / `NativeTick` / `OnConfirmClicked` 均在 **GameThread** 执行；Slate 输入事件处理也在 GameThread
4. **涉及 UObject 时怎么对 GC 可见？** `TitleLabel` 和 `ConfirmButton` 的 `meta=(BindWidget)` 隐含 `UPROPERTY` 语义；若持有其他辅助 `UObject*` 必须显式加 `UPROPERTY`，否则 GC 可能回收
5. **[本题专属]** `SObjectWidget` 同时持有 `TWeakObjectPtr<UUserWidget>` 和 `TSharedRef<SWidget>`，为什么是 `TWeakObjectPtr` 而不是 `TObjectPtr`？答：Slate 层持有 strong GC 引用会造成循环（UObject 的 Slate 树持有 UObject 本身），且 Slate 需要检测 UObject 是否已被 GC 回收后停止调用回调；`TWeakObjectPtr::IsValid()` 提供安全的有效性检查

## 对应官方参考

- `Engine/Source/Runtime/UMG/Public/Blueprint/UserWidget.h`（`NativePreConstruct` L1575，`NativeConstruct` L1576，`NativeTick` L1578）
- `Engine/Source/Runtime/UMG/Public/Components/Widget.h`（`meta=(BindWidget)` 注释 L70–80）
- `Engine/Source/Runtime/UMG/Public/Components/TextBlock.h`（`UTextBlock::SetText`）
- `Engine/Source/Runtime/UMG/Public/Components/Button.h`（`UButton::OnClicked`，`FOnButtonClickedEvent`）
