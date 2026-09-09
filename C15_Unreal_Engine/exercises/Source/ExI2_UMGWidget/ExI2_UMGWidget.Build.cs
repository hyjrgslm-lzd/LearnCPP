// 对应章节: ../../../11-模块I-Slate与UMG简介.md §练习 I-2
using UnrealBuildTool;

public class ExI2_UMGWidget : ModuleRules
{
    public ExI2_UMGWidget(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "Slate",
            "SlateCore",
        });

        // 教学重点: UMG UObject 外壳包裹 Slate 底层
        //   UUserWidget 继承链: UUserWidget → UWidget → UObject (GC 管理)
        //   meta=(BindWidget) 在 Initialize() 阶段填充子组件指针
        //   NativeConstruct 是运行时初始化钩子 (委托绑定在此进行)
        //   NativePreConstruct 是 Editor 设计时预览钩子 (禁止在此绑定委托)
    }
}
