// UELearn 习题集 Editor Target
// 对应教材: P:/C++Code/UELearn/README.md

using UnrealBuildTool;
using System.Collections.Generic;

public class UELearnEditorTarget : TargetRules
{
	public UELearnEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		ExtraModuleNames.AddRange(new string[]
		{
			"UELearn",
			"ExA1_HelloModule", "ExA2_LoadingPhase", "ExA3_PluginLayer",
			"ExB1_TArrayTMap", "ExB2_FString", "ExB3_FName",
			"ExC1_SharedPtr", "ExC2_Allocator", "ExC3_Delegate",
			"ExD1_HelloUObject", "ExD2_CDOSubobject", "ExD3_Reflection", "ExD4_MiniMarkSweep",
			"ExK1_HelloActor", "ExK2_ComponentCompose", "ExK3_WorldLevel", "ExK4_CustomSubsystem",
			"ExE1_Package", "ExE2_AssetRegistry", "ExE3_Streamable",
			"ExF1_Runnable", "ExF2_TaskGraph", "ExF3_UETasks", "ExF4_RenderCmd",
			"ExG1_RHIResource", "ExG2_GlobalShader", "ExG3_RHIPass",
			"ExH1_RDGDescribe", "ExH2_SceneProxy", "ExH3_ComputePass",
			"ExI1_SlateWidget", "ExI2_UMGWidget",
			"ExJ1_ClientServer", "ExJ2_Replication", "ExJ3_NetSerialize", "ExJ4_NetPrediction",
			"Cap1_UHeightField", "Cap2_SourceReading"
		});
	}
}
