// 对应章节: ../../../11-模块I-Slate与UMG简介.md §练习 I-1
using UnrealBuildTool;

public class ExI1_SlateWidget : ModuleRules
{
    public ExI1_SlateWidget(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "Slate",
            "SlateCore",
        });

        // 教学重点: 纯 C++ Slate widget, 无 UObject / 无 GC
        //   SCompoundWidget + SLATE_BEGIN_ARGS/SLATE_END_ARGS 宏块
        //   SLATE_ARGUMENT / SLATE_ATTRIBUTE / SLATE_EVENT 三者语义
        //   ChildSlot 布局与 TAttribute<FText> 惰性绑定
        // 不依赖 UMG / CoreUObject / Engine
    }
}
