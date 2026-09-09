// 对应章节: ../../../12-模块J-网络与Replication.md §练习 J-1
using UnrealBuildTool;

public class ExJ1_ClientServer : ModuleRules
{
    public ExJ1_ClientServer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "NetCore",    // 提供 FNetworkGUID / ENetRole / NetDriver 基础类型
        });

        // 教学重点: PIE 多人模式下 HasAuthority() 区分 Server / Client
        //   bReplicates = true 构造阶段标记, 使 Actor 加入 NetDriver 复制列表
        //   Play → Advanced Settings → Number of Players = 2 开多客户端 PIE
        //   BeginPlay 中 HasAuthority() 日志, 观察 Server / Client 窗口分别打印什么
    }
}
