// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B3
using UnrealBuildTool;

public class ExB3_FName : ModuleRules
{
	public ExB3_FName(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core"  // FName + FText (Internationalization) 均在 Core, LOCTEXT 宏亦在 Core
		});
	}
}
