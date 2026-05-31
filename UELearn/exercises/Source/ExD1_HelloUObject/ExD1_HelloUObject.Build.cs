// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-1
// 本题依赖:
//   Core        —— 基础类型、日志宏
//   CoreUObject —— UObject、UCLASS、UPROPERTY、UFUNCTION、UHT 代码生成
//   Engine      —— GetTransientPackage()、NewObject 依赖的 Engine 符号
using UnrealBuildTool;

public class ExD1_HelloUObject : ModuleRules
{
    public ExD1_HelloUObject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });

        // 学员实验: 注释掉 "CoreUObject" 观察链接失败 (StaticClass 等符号未解析)
        // 学员实验: 注释掉 "Engine" 观察 GetTransientPackage() 无法解析
    }
}
