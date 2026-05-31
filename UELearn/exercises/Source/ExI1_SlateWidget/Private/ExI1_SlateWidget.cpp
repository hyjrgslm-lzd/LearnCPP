// ============================================================
// 对应章节: ../../../11-模块I-Slate与UMG简介.md §练习 I-1
// C++ 标准要求: C++20
// 本题目标: 最小 SCompoundWidget — Construct / ChildSlot / TAttribute 惰性绑定
//
// 骨架阶段预期行为 (PIE 启动后 Output Log):
//   LogExI1: ExI1 SlateWidget 模块已启动
//   LogExI1: SExI1Widget::Construct 被调用 (骨架: ChildSlot 未挂载)
//
// 完成后预期行为:
//   屏幕上出现包含标题文本和计数文本的自定义 widget
//   每次调用 SetCounter(N) 后计数文本更新
// ============================================================

#include "ExI1_SlateWidget.h"
#include "Widgets/SBoxPanel.h"  // SVerticalBox / SHorizontalBox 定义于此
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FExI1SlateWidgetModule, ExI1_SlateWidget);

DEFINE_LOG_CATEGORY_STATIC(LogExI1, Log, All);

// ============================================================
// SExI1Widget 实现
// ============================================================

void SExI1Widget::Construct(const FArguments& InArgs)
{
    // Construct 是真正的初始化入口，此时 SharedThis(this) 已可用
    // 通过 ChildSlot[ ... ] 语法将根布局挂到本 widget

    UE_LOG(LogExI1, Log, TEXT("SExI1Widget::Construct 被调用"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 完成 ChildSlot 布局
    //
    // 目标布局:
    //   ChildSlot
    //   [
    //     SNew(SVerticalBox)
    //     + SVerticalBox::Slot().AutoHeight().Padding(4.f)
    //       [
    //         SNew(STextBlock)
    //           .Text(InArgs._Title)          // SLATE_ARGUMENT: 值绑定
    //       ]
    //     + SVerticalBox::Slot().AutoHeight().Padding(4.f)
    //       [
    //         SAssignNew(CounterText, STextBlock)
    //           .Text(InArgs._Counter)        // SLATE_ATTRIBUTE: 惰性绑定
    //       ]
    //   ]
    //
    // 关键: ChildSlot 只能挂一个根 widget (SCompoundWidget 的设计约束)
    //       多个子 widget 通过 SVerticalBox / SHorizontalBox 组合
    // ══════════════════════════════════════════════════════

    // 骨架阶段: 最小 ChildSlot，避免空 widget 崩溃
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SAssignNew(CounterText, STextBlock)
            .Text(InArgs._Title)
        ]
    ];

    // 保存 SLATE_ARGUMENT 值 (演示值已在 Construct 时确定，之后不再惰性求值)
    UE_LOG(LogExI1, Log, TEXT("  Title='%s'"), *InArgs._Title.ToString());

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 将 _Counter 改为 TAttribute 惰性绑定
    //
    // 步骤:
    //   在布局里对 CounterText 使用:
    //     .Text(TAttribute<FText>::Create(
    //         TAttribute<FText>::FGetter::CreateLambda(
    //             [this]() { return FText::AsNumber(CountValue); })))
    //
    //   然后调用 SetCounter(N) → CountValue = N
    //   观察: 无需手动调用 SetText，STextBlock 每帧 OnPaint 时通过 TAttribute::Get() 求值
    //   这就是 SLATE_ATTRIBUTE 的惰性求值：每次重绘时调用 getter，而不是存一个值副本
    // ══════════════════════════════════════════════════════

    // ══════════════════════════════════════════════════════
    // TODO [进阶] 1: 绑定 InArgs._OnTitleClicked 委托
    //   在 STextBlock 或 SButton 的 OnClicked 里 Execute OnTitleClicked
    //   演示 SLATE_EVENT 的委托传递模式
    // ══════════════════════════════════════════════════════
}

void SExI1Widget::SetCounter(int32 NewCount)
{
    CountValue = NewCount;

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2 的配套: 若使用直接值绑定 (非惰性)，在此手动调用 SetText
    //   if (CounterText.IsValid())
    //   {
    //       CounterText->SetText(FText::AsNumber(CountValue));
    //   }
    //
    // 若使用 TAttribute 惰性绑定 (推荐)，则无需任何操作:
    //   STextBlock 会在下次 OnPaint 时自动通过 getter 取得最新值
    // ══════════════════════════════════════════════════════

    UE_LOG(LogExI1, Log, TEXT("SExI1Widget::SetCounter(%d)"), CountValue);

    // 骨架阶段: 直接刷新 (完成 TODO 2 后可删除此行，改用惰性绑定)
    if (CounterText.IsValid())
    {
        CounterText->SetText(FText::AsNumber(CountValue));
    }
}

// ============================================================
// 模块生命周期
// ============================================================

void FExI1SlateWidgetModule::StartupModule()
{
    UE_LOG(LogExI1, Log, TEXT("ExI1 SlateWidget 模块已启动"));
    UE_LOG(LogExI1, Log, TEXT("  关键概念: SWidget 无 UObject / 无 GC, 全 TSharedPtr 管理"));
    UE_LOG(LogExI1, Log, TEXT("  SLATE_ARGUMENT = 值, SLATE_ATTRIBUTE = 惰性 getter, SLATE_EVENT = delegate"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 创建 widget 并显示
    //
    // 可选方式 A: 挂到 Editor 窗口 (需要 UnrealEd 模块依赖)
    //   FSlateApplication::Get().AddWindow(SNew(SWindow).Content(SNew(SExI1Widget)...));
    //
    // 可选方式 B: 嵌入 UMG (结合 I2):
    //   在 UExI2Widget::NativeConstruct 里:
    //   ChildSlot[ SNew(SExI1Widget).Title(NSLOCTEXT("ExI1", "T", "Hello Slate")) ]
    //
    // 可选方式 C: 通过 GameViewport 添加
    //   GEngine->GameViewport->AddViewportWidgetContent(SNew(SExI1Widget)...);
    // ══════════════════════════════════════════════════════
}

void FExI1SlateWidgetModule::ShutdownModule()
{
    UE_LOG(LogExI1, Log, TEXT("ExI1 SlateWidget 模块已卸载"));
}

// ---- 验证区 (完成 TODO 后把下列观察点写入 README.md 验收区) ----
// Q: 为什么 Construct 里必须通过 ChildSlot[] 而不是在构造函数里直接赋值？
// A: 构造函数调用时 TSharedRef 尚未完成初始化，SharedThis(this) 不可用，
//    AttachWidget (FSlotBase 内部) 需要 TSharedRef 有效才能安全执行。
//    Construct(const FArguments&) 在 SNew 内部、TSharedRef 初始化完成后调用。
//
// Q: SNew 返回的是什么类型？
// A: TSharedRef<SExI1Widget>。SNew 内部先用 new 分配，立即包装进 TSharedRef，
//    再调用 Construct。调用方不可对 Slate widget 持有裸指针或调用 delete。
