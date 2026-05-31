// 对应章节: ../../../04-模块C-内存-智能指针-委托.md §练习 C-3
// 本题依赖:
//   Core        —— DECLARE_DELEGATE_OneParam / DECLARE_MULTICAST_DELEGATE 等纯 C++ 委托宏
//   CoreUObject —— DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam 需要 UHT + UObject 体系
//                  (若进阶任务演示 dynamic delegate 时必须加 CoreUObject)
using UnrealBuildTool;

public class ExC3_Delegate : ModuleRules
{
    public ExC3_Delegate(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject"   // DYNAMIC_MULTICAST_DELEGATE 进阶部分需要; 注释掉可观察链接失败
        });
    }
}
