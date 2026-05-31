// ============================================================
// 对应章节: ../../../11-模块I-Slate与UMG简介.md §练习 I-2
// C++ 标准要求: C++20
// 本题目标: UMG + UUserWidget — meta=(BindWidget) + NativeConstruct
//
// 骨架阶段预期行为 (PIE 启动后 Output Log):
//   LogExI2: ExI2 UMGWidget 模块已启动
//   LogExI2: UExI2Widget::NativePreConstruct 被调用 (骨架: 仅打印日志)
//   LogExI2: UExI2Widget::NativeConstruct 被调用, TitleLabel 已填充
//
// 完成后预期行为:
//   屏幕上显示 BP_ExI2Widget，TitleLabel 显示 "ExI2 Widget"
//   点击 ConfirmButton 后 TitleLabel 文本更新
// ============================================================

#include "ExI2_UMGWidget.h"
#include "Components/Button.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, ExI2_UMGWidget);

DEFINE_LOG_CATEGORY_STATIC(LogExI2, Log, All);

// ============================================================
// UExI2Widget 实现
// ============================================================

void UExI2Widget::NativePreConstruct()
{
    // NativePreConstruct 在 Editor Widget Designer 每次属性变化时调用
    // 用途: 刷新设计时预览 (如设置占位文本)
    // 禁止: 绑定委托 (每次预览刷新都会重复添加)
    // 禁止: 有副作用的操作 (如 SpawnActor, 修改全局状态)

    Super::NativePreConstruct();

    UE_LOG(LogExI2, Log, TEXT("UExI2Widget::NativePreConstruct 被调用 (可能来自 Editor 预览)"));

    // 骨架阶段: 检查 TitleLabel 是否已填充
    // 注意: NativePreConstruct 在 Initialize() 之后被调用，TitleLabel 应已填充
    //       但在 Editor 设计时模式下，部分指针可能为 nullptr，需防御性检查
    if (TitleLabel)
    {
        TitleLabel->SetText(FText::FromString(TEXT("[预览] ExI2 Widget")));
    }
    else
    {
        UE_LOG(LogExI2, Warning,
            TEXT("  NativePreConstruct: TitleLabel 为 nullptr — BP 设计器里是否有同名控件?"));
    }
}

void UExI2Widget::NativeConstruct()
{
    // NativeConstruct 是运行时初始化钩子，类比 BeginPlay
    // 调用时机: CreateWidget → Initialize (填充 BindWidget 指针) → NativeConstruct
    //
    // 此时可以安全访问 meta=(BindWidget) 的子组件指针
    // 委托绑定必须在此进行

    // 必须调用 Super::NativeConstruct()，否则 BP Construct 事件不触发
    Super::NativeConstruct();

    UE_LOG(LogExI2, Log, TEXT("UExI2Widget::NativeConstruct 被调用 (运行时初始化)"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 1: 验证 TitleLabel 已填充，设置初始文本
    // ══════════════════════════════════════════════════════
    if (TitleLabel)
    {
        TitleLabel->SetText(FText::FromString(TEXT("ExI2 Widget")));
        UE_LOG(LogExI2, Log, TEXT("  TitleLabel 已填充, 初始文本已设置"));
    }
    else
    {
        UE_LOG(LogExI2, Error,
            TEXT("  TitleLabel 为 nullptr！请检查 BP 设计器里是否有名为 'TitleLabel' 的 UTextBlock"));
        // meta=(BindWidget) 填充失败时 UMG 会在 Editor 报错:
        // "Required widget binding 'TitleLabel' of class UTextBlock was not found"
    }

    // ══════════════════════════════════════════════════════
    // TODO [必做] 2: 绑定 ConfirmButton 的 OnClicked 委托
    //   取消下列注释 (同时在 .h 中取消 ConfirmButton 的声明注释):
    //
    // if (ConfirmButton)
    // {
    //     ConfirmButton->OnClicked.AddDynamic(this, &UExI2Widget::OnConfirmClicked);
    //     UE_LOG(LogExI2, Log, TEXT("  ConfirmButton OnClicked 委托已绑定"));
    // }
    // else
    // {
    //     UE_LOG(LogExI2, Error,
    //         TEXT("  ConfirmButton 为 nullptr！请检查 BP 设计器里是否有名为 'ConfirmButton' 的 UButton"));
    // }
    // ══════════════════════════════════════════════════════
}

void UExI2Widget::OnConfirmClicked()
{
    // 此函数在 GameThread 上执行 (Slate 输入事件 → UMG 动态委托 → GameThread)
    // 必须标记 UFUNCTION()，AddDynamic 通过函数名字符串注册，需要 UHT 反射元数据

    UE_LOG(LogExI2, Log, TEXT("UExI2Widget::OnConfirmClicked 被调用"));

    // ══════════════════════════════════════════════════════
    // TODO [必做] 3: 更新 TitleLabel 文本，演示点击响应
    //   if (TitleLabel)
    //   {
    //       static int32 ClickCount = 0;
    //       ++ClickCount;
    //       TitleLabel->SetText(FText::Format(
    //           NSLOCTEXT("ExI2", "Clicks", "已点击 {0} 次"), FText::AsNumber(ClickCount)));
    //   }
    // ══════════════════════════════════════════════════════
}

// ============================================================
// 模块生命周期
// ============================================================

// 注意: 模块注册使用 FDefaultModuleImpl
// UExI2Widget 通过 UHT 反射自动注册，不需要在 StartupModule 里手动操作

// ---- 验证区 (完成 TODO 后把下列问题答案写入 README.md) ----
// Q: 为什么 NativeConstruct 里可以安全访问 TitleLabel，构造函数里不能？
// A: meta=(BindWidget) 的填充发生在 UUserWidget::Initialize() 阶段。
//    Initialize() 在 CreateWidget 内部调用，晚于 UObject 构造函数。
//    因此构造函数执行时 TitleLabel 仍为 nullptr。
//    NativeConstruct 由 Initialize() 末尾触发，此时 BindWidget 已完成填充。
//
// Q: AddToViewport() 之前，Slate 层的 widget 树是否已建立？
// A: 未建立。UUserWidget 对象存在 (GC 可见)，但 Slate 树直到
//    AddToViewport() 触发 TakeWidget() → RebuildWidget() 才被构建。
//
// Q: 为什么 OnConfirmClicked 必须是 UFUNCTION()？
// A: UButton::OnClicked 是 FOnButtonClickedEvent (DECLARE_DYNAMIC_MULTICAST_DELEGATE)。
//    AddDynamic 宏通过函数名字符串在反射元数据里查找函数指针。
//    若漏掉 UFUNCTION()，UHT 不生成该函数的反射元数据，AddDynamic 静默失败。
