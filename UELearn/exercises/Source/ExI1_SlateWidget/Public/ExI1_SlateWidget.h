// 对应章节: ../../../11-模块I-Slate与UMG简介.md §练习 I-1
// 小节: 最小 SCompoundWidget — SLATE_BEGIN_ARGS / SLATE_ATTRIBUTE / ChildSlot
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

// ============================================================
// ExI1_SlateWidget 骨架
//
// 教学重点:
//   1. SCompoundWidget 是自定义纯 C++ Slate widget 的起点
//      无 UObject / 无 UCLASS / 无 GC — 完全由 TSharedPtr 引用计数管理
//   2. SLATE_BEGIN_ARGS / SLATE_END_ARGS 宏块展开产生 FArguments 嵌套结构体
//      链式调用 SNew(SExI1Widget).Title(NSLOCTEXT(...)) 就是在设置 FArguments 字段
//   3. SLATE_ARGUMENT  — 构造时一次性值 (不可绑定 lambda)
//   4. SLATE_ATTRIBUTE — TAttribute<T>，支持惰性绑定 (每次 Get() 才求值)
//   5. SLATE_EVENT     — delegate 回调
//   6. Construct(const FArguments&) 是真正的初始化入口
//      构造函数是 protected，只能通过 SNew/SAssignNew 创建
//      在 Construct 里通过 ChildSlot[ ... ] 挂布局
//
// 重要提示:
//   Slate widget 构造函数是 protected，SharedThis(this) 在构造函数里不可用。
//   所有子 widget 挂载必须在 Construct() 里完成，不得在构造函数里操作 ChildSlot。
//
// 源码校对路径:
//   Engine/Source/Runtime/SlateCore/Public/Widgets/SCompoundWidget.h
//   Engine/Source/Runtime/Slate/Public/Widgets/Text/STextBlock.h
//   Engine/Source/Runtime/SlateCore/Public/Misc/Attribute.h (TAttribute<T>)
// ============================================================

/**
 * SExI1Widget
 *
 * 最小自定义 SCompoundWidget，展示:
 *   - SLATE_ARGUMENT(FText, Title)    : 构造时传入一次的文本标题
 *   - SLATE_ATTRIBUTE(FText, Counter) : 可绑定 lambda 的计数文本 (惰性求值)
 *   - SLATE_EVENT(FSimpleDelegate, OnTitleClicked) : 点击标题时的回调
 *
 * 布局骨架 (Construct 里实现):
 *   ChildSlot
 *   [
 *     SNew(SVerticalBox)
 *     + SVerticalBox::Slot().AutoHeight()
 *       [ SNew(STextBlock).Text(InArgs._Title) ]          // 标题 (ARGUMENT)
 *     + SVerticalBox::Slot().AutoHeight()
 *       [ SNew(STextBlock).Text(InArgs._Counter) ]        // 计数 (ATTRIBUTE，惰性)
 *   ]
 */
class SExI1Widget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SExI1Widget)
        // SLATE_ARGUMENT: 构造时赋值一次，不支持后续惰性更新
        // 展开为 FArguments::_Title 字段 + .Title(FText) setter
        : _Title(FText::GetEmpty())
        // SLATE_ATTRIBUTE: 展开为 TAttribute<FText> 成员
        // 可以传值 .Counter(FText::FromString("0"))
        // 也可以传 lambda .Counter(TAttribute<FText>::Create([this]{ return ... }))
        , _Counter(FText::FromString(TEXT("0")))
        // SLATE_EVENT: 展开为 FSimpleDelegate 成员
        , _OnTitleClicked()
    {}
        SLATE_ARGUMENT(FText, Title)
        SLATE_ATTRIBUTE(FText, Counter)
        SLATE_EVENT(FSimpleDelegate, OnTitleClicked)
    SLATE_END_ARGS()

    /**
     * Construct
     *
     * SNew(SExI1Widget) 内部调用此函数完成初始化。
     * 此时 SharedThis(this) 已可用，可安全挂载子 widget。
     *
     * @param InArgs  由 SLATE_BEGIN_ARGS/END_ARGS 生成的 FArguments
     */
    void Construct(const FArguments& InArgs);

    // ── 公共接口 ──────────────────────────────────────────

    /** 更新计数文本 (演示 SLATE_ATTRIBUTE 与 TAttribute 惰性求值) */
    void SetCounter(int32 NewCount);

    // TODO [必做] 1: 在 Construct 里完成 ChildSlot 布局
    //               SNew(SVerticalBox) 内含 STextBlock(Title) + STextBlock(Counter)

    // TODO [必做] 2: 将 _Counter 改为绑定 lambda:
    //               InArgs._Counter 绑定 [this]() { return FText::AsNumber(CountValue); }
    //               观察 STextBlock 是否每帧自动刷新 (TAttribute 惰性求值路径)

    // TODO [进阶] 1: 阅读 SlateCore/Public/Misc/Attribute.h 中 TAttribute<T>::Get()
    //               理解惰性 getter 在 STextBlock::OnPaint 里何时被调用

private:
    /** 当前计数值 (演示 TAttribute 惰性绑定时使用) */
    int32 CountValue = 0;

    /** 持有 Counter STextBlock 的弱引用，用于直接刷新文本 */
    TSharedPtr<STextBlock> CounterText;
};

// ── 模块接口 ────────────────────────────────────────────

/** ExI1 模块接口 */
class FExI1SlateWidgetModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
