// 对应章节: ../../../05-模块D-UObject-反射-GC.md §练习 D-3
// 本题依赖:
//   Core        —— 基础类型、日志宏
//   CoreUObject —— UClass、FProperty、TFieldIterator、CastField<T>、UnrealType.h
//   Engine      —— GetTransientPackage()、ProcessEvent (UFUNCTION 反射调用)
using UnrealBuildTool;

public class ExD3_Reflection : ModuleRules
{
    public ExD3_Reflection(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine"
        });

        // TFieldIterator 定义在 CoreUObject/Public/UObject/FieldIterator.h
        // CastField<T>   定义在 CoreUObject/Public/UObject/FieldPath.h
        // FProperty      定义在 CoreUObject/Public/UObject/UnrealType.h
    }
}
