#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>

namespace ex = stdexec;

// ── 数据类型 ──────────────────────────────────────────
struct ParsedRecord {
    std::string field1;
    int         field2;
    double      field3;
};

struct ComputeResult {
    double normalized;
    double score;
};

// ── 辅助：线程安全打印 ───────────────────────────────
void log_phase(const char* phase) {
    std::ostringstream oss;
    oss << "  [" << phase << "] thread_id = "
        << std::this_thread::get_id() << "\n";
    std::cout << oss.str();
}

int main() {
    std::cout << "主线程 thread_id = " << std::this_thread::get_id() << "\n\n";

    // ── 创建两个线程池 ────────────────────────────────
    exec::static_thread_pool parse_pool(2);
    exec::static_thread_pool compute_pool(2);
    auto parse_sch   = parse_pool.get_scheduler();
    auto compute_sch = compute_pool.get_scheduler();

    // 原始输入
    std::string raw_input = "sensor_a,42,3.14";

    // ══════════════════════════════════════════════════════
    // Phase A: 在 parse_pool 上解析
    // ══════════════════════════════════════════════════════
    std::cout << "--- Phase A + B (starts_on 版本) ---\n";

    // TODO [必做]: 构造 sender 图
    //   1. 用 ex::starts_on(parse_sch, ...) 让解析工作在 parse_pool 上执行
    //   2. 在 then 中把 raw_input 解析成 ParsedRecord
    //      （简化：直接手工构造即可，不需要真正解析字符串）
    //   3. 打印线程 ID 以证明在 parse_pool 上
    //
    // auto parse_sender = ex::starts_on(parse_sch,
    //     ex::just(raw_input)
    //     | ex::then([](std::string raw) -> ParsedRecord {
    //         log_phase("parse");
    //         // TODO [必做]: 解析 raw 为 ParsedRecord
    //         return ParsedRecord{"sensor_a", 42, 3.14};
    //     })
    // );

    // ══════════════════════════════════════════════════════
    // Phase B: 切换到 compute_pool 做计算 (starts_on 版本)
    // ══════════════════════════════════════════════════════

    // TODO [必做]: 用 let_value 拿到 ParsedRecord 后，
    //   返回 starts_on(compute_sch, ...) 的新 sender，
    //   在 then 中做计算并打印线程 ID。
    //
    // auto full_pipeline_v1 = std::move(parse_sender)
    //     | ex::let_value([&compute_sch](ParsedRecord& rec) {
    //         return ex::starts_on(compute_sch,
    //             ex::just(std::move(rec))
    //             | ex::then([](ParsedRecord rec) -> ComputeResult {
    //                 log_phase("compute (starts_on)");
    //                 // TODO [必做]: 计算 normalized 和 score
    //                 double normalized = rec.field3 / 100.0;
    //                 double score = rec.field2 * normalized;
    //                 return ComputeResult{normalized, score};
    //             })
    //         );
    //     });
    //
    // auto [result_v1] = ex::sync_wait(std::move(full_pipeline_v1)).value();

    ComputeResult result_v1{0.0, 0.0};  // 替换为 sync_wait 结果

    std::cout << "  [result_v1] normalized=" << result_v1.normalized
              << ", score=" << result_v1.score << "\n\n";

    // ══════════════════════════════════════════════════════
    // Phase C: continues_on 版本
    // ══════════════════════════════════════════════════════
    std::cout << "--- Phase A + B (continues_on 版本) ---\n";

    // TODO [必做]: 用 continues_on 替代 starts_on + let_value
    //   语义差异：continues_on 控制的是"当前 sender 完成后，
    //   后续工作在哪里继续"。
    //
    // auto full_pipeline_v2 = ex::starts_on(parse_sch,
    //     ex::just(raw_input)
    //     | ex::then([](std::string raw) -> ParsedRecord {
    //         log_phase("parse");
    //         return ParsedRecord{"sensor_a", 42, 3.14};
    //     })
    //     | ex::continues_on(compute_sch)
    //     | ex::then([](ParsedRecord rec) -> ComputeResult {
    //         log_phase("compute (continues_on)");
    //         double normalized = rec.field3 / 100.0;
    //         double score = rec.field2 * normalized;
    //         return ComputeResult{normalized, score};
    //     })
    // );
    //
    // auto [result_v2] = ex::sync_wait(std::move(full_pipeline_v2)).value();

    ComputeResult result_v2{0.0, 0.0};  // 替换为 sync_wait 结果

    std::cout << "  [result_v2] normalized=" << result_v2.normalized
              << ", score=" << result_v2.score << "\n\n";

    // ── 验证 ──────────────────────────────────────────
    std::cout << "[验证] 两个版本结果应一致:\n";
    std::cout << "  v1: normalized=" << result_v1.normalized
              << ", score=" << result_v1.score << "\n";
    std::cout << "  v2: normalized=" << result_v2.normalized
              << ", score=" << result_v2.score << "\n";

    // ══════════════════════════════════════════════════════
    // TODO [进阶]: 把阶段 B 再拆成 enrich 与 compute_stats 两个小阶段。
    // TODO [进阶]: 做一个反例版本：故意把阶段 B 写回 parse_pool，
    //   比较日志差异。
    // ══════════════════════════════════════════════════════

    return 0;
}
