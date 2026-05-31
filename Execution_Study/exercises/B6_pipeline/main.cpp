#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>

namespace ex = stdexec;

// ── 数据类型 ──────────────────────────────────────────
struct RawRecord {
    std::string text;
};

struct ParsedRecord {
    std::string name;
    int         value;
    bool        valid;
};

struct EnrichedRecord {
    ParsedRecord base;
    double       derived_score;
};

struct Report {
    int    total;
    int    valid_count;
    double total_score;
};

// ── 三份原始输入 ──────────────────────────────────────
static const RawRecord raw_inputs[] = {
    {"Alice,100"},
    {"Bob,200"},
    {"Charlie,150"},
};

// ── 辅助：线程安全打印 ───────────────────────────────
void log_stage(const char* stage, const char* record_name) {
    std::ostringstream oss;
    oss << "  [" << stage << "] record=\"" << record_name
        << "\" thread_id=" << std::this_thread::get_id() << "\n";
    std::cout << oss.str();
}

// ══════════════════════════════════════════════════════
// parse: RawRecord -> ParsedRecord
// ══════════════════════════════════════════════════════
ParsedRecord parse(const RawRecord& raw) {
    // TODO [必做]: 把 raw.text 解析为 ParsedRecord
    //   简化做法：手工拆分逗号前后部分
    //   例如 "Alice,100" -> ParsedRecord{"Alice", 100, true}
    //   解析失败时可设 valid = false

    log_stage("parse", raw.text.c_str());

    // 占位实现 —— 请替换为真正的解析逻辑
    return ParsedRecord{"", 0, false};
}

// ══════════════════════════════════════════════════════
// enrich: ParsedRecord -> EnrichedRecord
// ══════════════════════════════════════════════════════
EnrichedRecord enrich(ParsedRecord rec) {
    // TODO [必做]: 为 ParsedRecord 补充派生字段 derived_score
    //   例如 derived_score = rec.value * 1.5

    log_stage("enrich", rec.name.c_str());

    // 占位实现 —— 请替换
    return EnrichedRecord{std::move(rec), 0.0};
}

// ══════════════════════════════════════════════════════
// 构造单条记录的 sender 分支: parse -> enrich
// ══════════════════════════════════════════════════════
auto make_branch(const RawRecord& raw, auto sch) {
    // TODO [必做]: 在 sch 上启动，先 parse 再 enrich
    //   返回一个产出 EnrichedRecord 的 sender
    //
    // return ex::starts_on(sch,
    //     ex::just(raw)
    //     | ex::then([](RawRecord r) { return parse(r); })
    //     | ex::then([](ParsedRecord p) { return enrich(std::move(p)); })
    // );

    // 占位：直接在 inline 上下文返回
    return ex::just(raw)
        | ex::then([](RawRecord r) { return parse(r); })
        | ex::then([](ParsedRecord p) { return enrich(std::move(p)); });
}

int main() {
    std::cout << "主线程 thread_id = " << std::this_thread::get_id() << "\n\n";

    // ── 线程池 ────────────────────────────────────────
    exec::static_thread_pool pool(2);
    auto sch = pool.get_scheduler();

    // ══════════════════════════════════════════════════════
    // TODO [必做]: 用 when_all 汇合三条 make_branch 分支，
    //   在最后的 then 中把三个 EnrichedRecord 合并成 Report。
    //
    // auto pipeline = ex::when_all(
    //         make_branch(raw_inputs[0], sch),
    //         make_branch(raw_inputs[1], sch),
    //         make_branch(raw_inputs[2], sch)
    //     )
    //     | ex::then([](EnrichedRecord a, EnrichedRecord b, EnrichedRecord c)
    //                    -> Report {
    //         // TODO [必做]: 汇总成 Report
    //         int total = 3;
    //         int valid_count = (a.base.valid ? 1 : 0)
    //                         + (b.base.valid ? 1 : 0)
    //                         + (c.base.valid ? 1 : 0);
    //         double total_score = a.derived_score
    //                            + b.derived_score
    //                            + c.derived_score;
    //         return Report{total, valid_count, total_score};
    //     });
    //
    // auto [report] = ex::sync_wait(std::move(pipeline)).value();
    // ══════════════════════════════════════════════════════

    // 临时占位，完成 TODO 后删除
    Report report{0, 0, 0.0};

    // ── 打印最终报告 ──────────────────────────────────
    std::cout << "\n=== Final Report ===\n";
    std::cout << "  total        = " << report.total       << "\n";
    std::cout << "  valid_count  = " << report.valid_count << "\n";
    std::cout << "  total_score  = " << report.total_score << "\n";

    // ══════════════════════════════════════════════════════
    // TODO [进阶]: 增加一个"过滤异常记录"的子阶段（纯 value path）。
    // TODO [进阶]: 把 merge 拆成"汇总统计"和"生成展示对象"两阶段。
    // TODO [进阶]: 把三条固定分支升级为可扩展的 N 条分支。
    // ══════════════════════════════════════════════════════

    return 0;
}
