// 占位主模块: 仅负责 IMPLEMENT_PRIMARY_GAME_MODULE, 不含习题代码
// 对应教材: P:/C++Code/UELearn/01-心智模型.md §模块先于文件

using UnrealBuildTool;

public class UELearn : ModuleRules
{
	public UELearn(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	}
}
