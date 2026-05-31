// 对应章节: ../../../02-模块A-构建系统与模块生命周期.md §练习 A-2
using UnrealBuildTool;

public class ExA2_LoadingPhase : ModuleRules
{
    public ExA2_LoadingPhase(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject"   // 用于 UObject::StaticClass() 探测 UObject 系统就绪状态
        });

        // PrivateDependencyModuleNames: 按需追加
        // 注意: LoadingPhase 是运行时加载顺序，与 .Build.cs 依赖无关
        //       修改 .uproject 里的 LoadingPhase 不需要重新跑 UBT
    }
}
