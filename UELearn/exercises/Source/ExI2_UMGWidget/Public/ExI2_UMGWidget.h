// 对应章节: ../../../11-模块I-Slate与UMG简介.md §练习 I-2
// 小节: UMG + UUserWidget (BP 绑定) — meta=(BindWidget) + NativeConstruct
// C++ 标准要求: C++20
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
// 注意: 不需要 include Button.h — 通过前向声明 + UPROPERTY 宏即可，
//       UButton 的完整定义在 .cpp 里按需引入
class UButton;

#include "ExI2_UMGWidget.generated.h"

// ============================================================
// ExI2_UMGWidget 骨架
//
// 教学重点:
//   1. UUserWidget 是 UMG 面向蓝图的主入口
//      继承链: UUserWidget → UWidget → UObject (GC 管理)
//      与 SCompoundWidget (I1) 完全不同的生命周期路径
//
//   2. meta=(BindWidget)
//      UMG 框架在 Initialize() 阶段通过反射查找 BP 设计器中同名 widget
//      填充对应 UPROPERTY 指针。若 BP 里没有同名控件，Editor 报错。
//      在 C++ 构造函数里访问这些指针 → nullptr 崩溃 (最高频坑!)
//
//   3. NativeConstruct  — 运行时初始化钩子 (类比 BeginPlay)
//      委托绑定 / 子组件访问必须在此进行，不得在构造函数里操作
//   4. NativePreConstruct — Editor 设计时预览钩子
//      每次属性变化都会调用，禁止在此绑定委托 (会重复添加)
//
//   5. AddDynamic 绑定的函数必须是 UFUNCTION()
//      UMG 委托是 FDynamicMulticastDelegate，需要 UHT 反射元数据
//
// BP 子类创建步骤 (README 有详细指引):
//   Editor → 右键 → User Interface → Widget Blueprint
//   父类选 UExI2Widget，命名 BP_ExI2Widget
//   在 Widget Designer 里添加 UTextBlock 命名 TitleLabel
//
// 源码校对路径:
//   Engine/Source/Runtime/UMG/Public/Blueprint/UserWidget.h
//   Engine/Source/Runtime/UMG/Public/Components/TextBlock.h
//   Engine/Source/Runtime/UMG/Public/Components/Button.h
// ============================================================

/**
 * UExI2Widget
 *
 * UUserWidget 派生类，演示:
 *   - meta=(BindWidget) 绑定 BP 设计器中的同名控件
 *   - NativeConstruct 里安全访问子组件、绑定委托
 *   - NativePreConstruct 只做轻量预览初始化
 *   - UFUNCTION() OnButtonClicked 作为动态委托回调
 */
UCLASS()
class EXI2_UMGWIDGET_API UExI2Widget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ── UUserWidget 钩子 ──────────────────────────────────

    /**
     * NativePreConstruct
     *
     * 在 Editor Widget Designer 的每次预览刷新时调用。
     * 只做轻量的"设计时可见"初始化（如设置默认文本）。
     * 禁止在此绑定委托（每次刷新都会重复添加）。
     */
    virtual void NativePreConstruct() override;

    /**
     * NativeConstruct
     *
     * 运行时初始化钩子，类比 BeginPlay。
     * 此时 meta=(BindWidget) 指针已由 Initialize() 填充，可安全访问。
     * 委托绑定必须在此进行。
     *
     * 调用顺序: CreateWidget → Initialize → NativeConstruct → BP Construct 事件
     *
     * 重要: 必须调用 Super::NativeConstruct()，否则 BP 的 Construct 事件不触发。
     */
    virtual void NativeConstruct() override;

protected:
    // ── meta=(BindWidget) 子组件 ──────────────────────────
    // 变量名必须与 BP Widget Designer 中控件名完全一致 (大小写敏感)
    // 在构造函数里访问这些指针 → nullptr，只在 NativeConstruct 及之后访问

    /**
     * TitleLabel
     *
     * BP 设计器里必须有一个名为 "TitleLabel" 的 UTextBlock。
     * meta=(BindWidget) 让 UMG 框架在 Initialize() 时自动填充此指针。
     *
     * TODO [必做] 1: 在 NativeConstruct 里调用 TitleLabel->SetText(FText::FromString(TEXT("ExI2")))
     *               验证 meta=(BindWidget) 填充正常，无 nullptr 崩溃
     */
    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> TitleLabel;

    // TODO [必做] 2: 在 BP 设计器里添加 UButton 命名 "ConfirmButton"，
    //               在此声明 UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ConfirmButton;
    //               在 NativeConstruct 里绑定 ConfirmButton->OnClicked.AddDynamic(this, &UExI2Widget::OnConfirmClicked);
    // UPROPERTY(meta=(BindWidget))
    // TObjectPtr<UButton> ConfirmButton;

private:
    // ── 内部回调 ──────────────────────────────────────────

    /**
     * OnConfirmClicked
     *
     * 动态委托回调，必须标记 UFUNCTION()。
     * UButton::OnClicked 是 FOnButtonClickedEvent (DECLARE_DYNAMIC_MULTICAST_DELEGATE)，
     * AddDynamic 通过函数名字符串注册，需要 UHT 产生的反射元数据。
     *
     * TODO [必做] 3: 实现此函数，更新 TitleLabel 文本，演示点击响应
     */
    UFUNCTION()
    void OnConfirmClicked();

    // TODO [进阶] 1: 覆盖 NativeTick，每帧打印 GEngine->AddOnScreenDebugMessage
    //               注意: UUserWidget 默认 meta=(DisableNativeTick)，
    //               需要覆盖 GetDesiredTickFrequency() 返回 EWidgetTickFrequency::Auto

    // TODO [进阶] 2: 阅读 Engine/Source/Runtime/UMG/Private/Components/Widget.cpp
    //               的 RebuildWidget()，在调试器里观察 Slate 树的构建时机
    //               (AddToViewport() 触发 TakeWidget() → RebuildWidget())
};
