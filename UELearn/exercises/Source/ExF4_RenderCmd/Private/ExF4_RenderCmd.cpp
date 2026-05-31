// ============================================================
// 对应章节: ../../../08-模块F-并发与任务图.md §练习 F-4
// C++ 标准要求: C++20
// 本题目标: ENQUEUE_RENDER_COMMAND 值捕获语义演示 + 悬垂引用反面教材
//
// 教学核心（必须内化）：
//   render command lambda 必须按值捕获所有数据。
//   构造时 GameThread 栈帧执行完毕后局部变量已析构；
//   lambda 在 RenderThread 执行时如果捕获了引用，读到的是悬空内存。
//
// 骨架阶段预期行为: 模块注册，StartupModule 打印 GameThread ID
// 完成后预期行为（Output Log）:
//   LogExF4: [GameThread] Enqueued，ThreadId=<GT>
//   LogExF4: [RenderThread] FrameValue=<N>，ThreadId=<RT>（与 GT 不同）
// ============================================================

#include "ExF4_RenderCmd.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformTLS.h"
#include "HAL/PlatformProcess.h"
#include "RenderingThread.h"
#include "RHICommandList.h"

DEFINE_LOG_CATEGORY_STATIC(LogExF4, Log, All);

class FExF4Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        const uint32 GameTid = FPlatformTLS::GetCurrentThreadId();
        UE_LOG(LogExF4, Log, TEXT("[GameThread] ThreadId=%u，开始 ENQUEUE_RENDER_COMMAND 演示"), GameTid);

        // ═══════════════════════════════════════════════════════════════
        // 正确示范：按值捕获 GameThread 局部变量
        //
        // 关键：ValueSnapshot 是值拷贝，lambda 持有独立副本。
        // 即使 StartupModule 栈帧返回，lambda 内的 ValueSnapshot 仍有效。
        // ═══════════════════════════════════════════════════════════════
        const int32 ValueSnapshot = 42; // 模拟帧号或任意 POD 数据

        ENQUEUE_RENDER_COMMAND(ExF4_SafeCapture)(
            [ValueSnapshot, GameTid](FRHICommandListImmediate& RHICmdList)
            // ^^^^^^^^^^^^^^ 按值捕获：lambda 持有独立副本，RenderThread 执行时完全安全
            {
                const uint32 RenderTid = FPlatformTLS::GetCurrentThreadId();
                UE_LOG(LogExF4, Log,
                    TEXT("[RenderThread] 值捕获示范：ValueSnapshot=%d，ThreadId=%u（GameTid=%u）"),
                    ValueSnapshot, RenderTid, GameTid);
                // 验证：RenderTid 应与 GameTid 不同（GIsThreadedRendering = true 时）
            }
        );

        // GameThread 侧继续往下走，lambda 尚未执行
        UE_LOG(LogExF4, Log, TEXT("[GameThread] Enqueued 完成，GameThread 继续执行（lambda 尚未在 RenderThread 运行）"));

        // ═══════════════════════════════════════════════════════════════
        // 故意错误示范（引用捕获 = 悬垂指针，注释掉防止实际运行）
        //
        // 以下代码演示最高频 bug：按引用捕获 GameThread 局部变量
        // ★ 禁止取消注释此块并实际运行 ★
        // ═══════════════════════════════════════════════════════════════
        // int32 LocalValue = 99; // GameThread 栈变量
        //
        // ENQUEUE_RENDER_COMMAND(ExF4_DangerousRef)(
        //     [&LocalValue](FRHICommandListImmediate& RHICmdList)
        //     // ^^^^^^^^^ 按引用捕获！
        //     // StartupModule 返回后 LocalValue 已在栈上析构
        //     // RenderThread 执行此处时读到的是垃圾值或触发访问违例（UAF）
        //     {
        //         UE_LOG(LogExF4, Error, TEXT("[RenderThread] 悬垂引用！LocalValue=%d"), LocalValue);
        //     }
        // );

        // ══════════════════════════════════════════════════════
        // TODO [必做] 1: FlushRenderingCommands 对比实验
        //   版本 A：不 Flush，观察打印时机
        //   版本 B：取消下行注释，观察帧卡顿与打印时机变化
        //   参考: Engine/Source/Runtime/RenderCore/Public/RenderingThread.h 第 110 行
        // ══════════════════════════════════════════════════════
        // FlushRenderingCommands(); // 版本 B：阻塞 GameThread 直到 RenderThread 排空命令队列

        // ══════════════════════════════════════════════════════
        // TODO [必做] 2: 观察 GameThread 在 Enqueue 后继续执行，
        //   用 FPlatformProcess::Sleep(0) 或简单计数演示"Enqueued"先于"RenderThread 日志"打印
        // ══════════════════════════════════════════════════════

        // ──────────────────────────────────────────────────
        // TODO [进阶] 1: 在 TickComponent 里每帧 Enqueue 命令，打印帧号与时间戳，
        //               观察帧号与执行时间戳之间的 1 帧延迟规律
        // TODO [进阶] 2: 研究 DECLARE_RENDER_COMMAND_TAG 宏展开（RenderingThread.h 第 240 行）
        //               理解 stat 数据如何随 render command 传递
        // ──────────────────────────────────────────────────

        UE_LOG(LogExF4, Log, TEXT("[GameThread] ExF4_RenderCmd StartupModule 返回，render command 将在 RenderThread 稍后执行"));
    }

    virtual void ShutdownModule() override
    {
        // 关卡卸载时可以 FlushRenderingCommands()（这里是合法场景，不在 Tick 里）
        FlushRenderingCommands();
        UE_LOG(LogExF4, Log, TEXT("[GameThread] ExF4_RenderCmd 模块卸载，RenderThread 命令已排空"));
    }
};

IMPLEMENT_MODULE(FExF4Module, ExF4_RenderCmd);

// ---- 验证区 ----
// 运行后在 Output Log 过滤 LogExF4：
//   - "[GameThread] Enqueued" 必须先于 "[RenderThread]" 出现
//   - 两条日志的 ThreadId 必须不同（GIsThreadedRendering=true 时）
//   - FlushRenderingCommands 版本（版本 B）能感知明显等待延迟
