// ============================================================
// 对应章节: ../../../10-模块H-RenderGraph与场景渲染.md §练习 H-1
// C++ 标准要求: C++20
// 本题目标: RDG 惰性图 inspect + stdexec 类比表 (纯阅读分析题)
//
// 骨架阶段预期行为: Module 注册，PIE 启动打印一条日志
// 完成后预期行为 (阅读 + 笔记，无新的运行时输出):
//   LogExH1: ExH1 模块已注册 — 开始阅读 RenderGraphBuilder.h 第 42-65 行
//   LogExH1: 参考三阶段注释 (描述期 / 编译期 / 执行期)
// ============================================================

#include "ExH1_RDGDescribe.h"
#include "Modules/ModuleManager.h"

// 每题独立 log category，过滤时使用 LogExH1
DEFINE_LOG_CATEGORY_STATIC(LogExH1, Log, All);

IMPLEMENT_MODULE(FDefaultModuleImpl, ExH1_RDGDescribe);

void FExH1RDGDescribeModule::StartupModule()
{
    UE_LOG(LogExH1, Log, TEXT("ExH1 RDGDescribe 模块已启动"));
    UE_LOG(LogExH1, Log, TEXT("--- 阅读任务提示 ---"));
    UE_LOG(LogExH1, Log, TEXT("1. 打开 Engine/Source/Runtime/RenderCore/Public/RenderGraphBuilder.h 第 42-65 行"));
    UE_LOG(LogExH1, Log, TEXT("   关注: 构造函数只初始化 allocator, 无任何 GPU 命令"));
    UE_LOG(LogExH1, Log, TEXT("2. 阅读 AddPass 声明 (第 200-230 行): lambda 被存储, 不立即调用"));
    UE_LOG(LogExH1, Log, TEXT("3. 阅读 RenderGraphBuilder.cpp 第 1316 行 Compile() 三阶段"));
    UE_LOG(LogExH1, Log, TEXT("4. 完成 README.md 中 RDG <-> stdexec 六维类比表"));
}

void FExH1RDGDescribeModule::ShutdownModule()
{
    UE_LOG(LogExH1, Log, TEXT("ExH1 RDGDescribe 模块已卸载"));
}

// ============================================================
// RDG 三阶段注释 (供阅读参考, 对应教材图示)
//
// 描述期 (FRDGBuilder 建图):
//   FRDGBuilder GraphBuilder(RHICmdList);                       // 只初始化 CPU 状态
//   FRDGTextureRef RT = GraphBuilder.CreateTexture(Desc, ...); // 只创建元数据句柄
//   GraphBuilder.AddPass(                                       // lambda 存入 Passes[], 不调用
//       RDG_EVENT_NAME("ExH1"),
//       PassParameters,
//       ERDGPassFlags::Raster,
//       [](FRHICommandList& RHICmdList) { /* GPU 命令在此，但现在不执行 */ });
//
//          │
//          ▼ Execute() 被调用
//
// 编译期 (FRDGBuilder::Compile()):
//   SetupPassDependencies()  → 建立 producer/consumer 边, 计算引用计数
//   Pass culling             → 裁剪输出未被消费的 pass (等价于惰性 sender 未被 start)
//   Barrier 推导             → 遍历资源读写序列, 在正确位置插入 FRHITransition
//   Transient lifetime 计算 → 确定每个资源的 first-writer / last-reader pass
//   GPU 显存分配             → 此刻 FRDGTextureRef 对应的 FRHITexture 才真正存在
//
//          │
//          ▼ pass 遍历
//
// 执行期 (遍历 Passes, 调用 lambda):
//   Pass[0].Lambda(RHICmdList) → 录制第一个 pass 的 GPU 命令
//   <RDG 自动插入 barrier>     → 资源状态 RT → SRV
//   Pass[1].Lambda(RHICmdList) → 录制第二个 pass
//   ...
// ============================================================

// ============================================================
// RDG <-> stdexec 六维类比表 (学员完成版在 README.md)
//
// | 维度           | stdexec                        | UE RDG                              |
// |----------------|--------------------------------|-------------------------------------|
// | 描述期         | just(x) / then(s, f) 构造图    | FRDGBuilder::AddPass(...)           |
// | 执行启动       | start(op) / sync_wait(s)        | FRDGBuilder::Execute()              |
// | 生命周期拥有者 | operation_state                 | FRDGBuilder (栈对象 + transient 池) |
// | 依赖/类型分析  | completion_signatures (编译期)  | Compile() 运行时依赖分析            |
// | 图节点         | sender adaptor (then/when_all)  | FRDGPass (lambda + 参数元数据)      |
// | 资源依赖流     | tag_invoke / env 值传播         | FRDGTextureRef / FRDGBufferRef      |
// ============================================================

// ══════════════════════════════════════════════════════
// TODO [必做] 1: 阅读 RenderGraphBuilder.h 并在 README.md 中填写六维类比表
//               每行 "说明" 列用自己的话写，不得照抄本注释
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [必做] 2: 手动推导三 pass pipeline 的 barrier + lifetime
//               (见教材 §练习 H-1 任务 4: Heightfield → Normal → Gamma)
//   TextureA: first-writer=PassA, last-reader=PassB → PassA 前分配, PassB 后归还
//   TextureB: first-writer=PassB, last-reader=PassC → PassB 前分配, PassC 后归还
//   TextureC: first-writer=PassC, last-reader=QueueTextureExtraction → 持有至帧结束
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 1: 阅读 RenderGraphBuilder.cpp 第 1316 行 Compile() 中
//               SetupPassDependencies() 的引用计数累加逻辑
//               找到 PassState.Texture->ReferenceCount += PassState.ReferenceCount
// ══════════════════════════════════════════════════════

// ══════════════════════════════════════════════════════
// TODO [进阶] 2: 找到 Execute() 中 bCullPasses 判断分支
//               理解 GRDGCullPasses console variable 如何控制 pass culling
// ══════════════════════════════════════════════════════

// ---- 验证区 (完成 TODO 后移入笔记/README) ----
// Q: FRDGTextureRef 在描述期代表什么？在执行期代表什么？
// A(描述期): FRDGTexture* 指向纯 CPU 元数据对象, 无 GPU 内存
// A(执行期): Compile() 后 transient pool 已分配 FRHITexture, GetRHI() 才有效
