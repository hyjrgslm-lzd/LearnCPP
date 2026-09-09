// 对应章节: ../../../03-模块B-核心容器与字符串.md §练习 B2
using UnrealBuildTool;

public class ExB2_FString : ModuleRules
{
	public ExB2_FString(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core"  // FString / FStringView / TCHAR_TO_UTF8 均在 Core
		});
	}
}
