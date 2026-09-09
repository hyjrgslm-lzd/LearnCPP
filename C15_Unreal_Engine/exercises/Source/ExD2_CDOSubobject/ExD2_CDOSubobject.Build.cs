// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-2
// 本题依赖:
//   Core        —— 基础类型、日志宏
//   CoreUObject —— UObject、FObjectInitializer、CreateDefaultSubobject
//   Engine      —— GetTransientPackage()、Actor 相关 (若选用 AActor 路径)
using UnrealBuildTool;

public class ExD2_CDOSubobject : ModuleRules
{
    public ExD2_CDOSubobject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });

        // 学员实验: 注释掉 "Engine" 后 GetTransientPackage() 报符号缺失
        // 观察: CreateDefaultSubobject 在 CoreUObject 中声明
        //       (UObjectGlobals.h:1363), Engine 不是必需依赖但实践中常用
    }
}
