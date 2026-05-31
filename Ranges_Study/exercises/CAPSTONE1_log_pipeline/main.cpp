// 结课项目 1：CPU 数据流水线 — CSV 日志多层管道
// 章节：07-结课项目1-数据管道与源码阅读.md §结课项目 1
// 目标：把模块 A-D 的主要能力串起来，做一个完整的 ranges 小系统
// C++ 基线：C++20（ranges::to / chunk_by / zip 为 C++23，需编译器支持）
//
// 默认输出：
//   CAPSTONE1 skeleton — fill TODOs to build pipeline
//
// 输入格式（每条记录一行）：
//   timestamp,level,user_id,message
//   level ∈ {INFO, WARN, ERROR}；非法行产出 nullopt，被 filter 过滤

#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// ---- 示例日志（学员可在此修改输入内容）----
constexpr std::string_view sample_log =
    "2024-01-01T00:00:01,INFO,alice,login ok\n"
    "2024-01-01T00:00:02,WARN,bob,disk usage 80%\n"
    "2024-01-01T00:00:03,ERROR,alice,connection refused\n"
    "2024-01-01T00:00:04,ERROR,carol,timeout\n"
    "INVALID_LINE_NO_COMMAS\n"
    "2024-01-01T00:00:05,INFO,bob,task complete\n"
    "2024-01-01T00:00:06,ERROR,alice,segfault detected\n"
    "2024-01-01T00:00:07,BADLEVEL,dave,unknown\n"
    "2024-01-01T00:00:08,WARN,carol,retry 2\n";

// ---- 数据结构 ----
enum class Level { INFO, WARN, ERROR, UNKNOWN };

struct LogRecord {
    std::string timestamp;
    Level       level;
    std::string user_id;
    std::string message;
};

// ============================================================
// TODO [必做] 1: 先画 view 依赖图，再写代码
//   至少标出 4 层的 iterator_concept 和是否 borrowed。
//   图存档方式：在此处写文本注释块，例如：
//
//   raw_lines (vector<string_view>)   contiguous_range, borrowed
//     └─ transform(parse_line)        random_access_range, NOT borrowed
//          └─ filter(has_value)       bidirectional_range, NOT borrowed
//               └─ transform(unwrap) bidirectional_range, NOT borrowed
//
//   borrowed_range 验证示例（填完后解注释）：
//   static_assert( std::ranges::borrowed_range<decltype(std::views::iota(0,10))>);
//   static_assert(!std::ranges::borrowed_range<
//       decltype(std::vector<std::string_view>{} | std::views::filter([](auto){ return true; }))>);
// ============================================================

// ============================================================
// TODO [必做] 2: 解析阶段
//   实现 parse_line(std::string_view line) -> std::optional<LogRecord>
//   逻辑：按逗号分割出 4 个字段；字段不足 4 个或 level 不识别 -> nullopt
//   管道：
//     raw_lines
//       | views::transform(parse_line)    -> range<optional<LogRecord>>
//       | views::filter(&optional::has_value 或 lambda)
//       | views::transform([](auto& o){ return *o; })
//       -> range<LogRecord>
// ============================================================

// ============================================================
// TODO [必做] 3: 按 level 分组计数（projection）
//   用 ranges::count_if 分别统计 INFO / WARN / ERROR：
//
//   auto err_count = std::ranges::count_if(
//       records,
//       [](Level l){ return l == Level::ERROR; },
//       &LogRecord::level    // projection：先取 level 再传给谓词
//   );
//
//   打印三种 level 的计数
// ============================================================

// ============================================================
// TODO [必做] 4: 按 user_id 分组计数（ranges::to 收束）
//   方式 A：手动 fold 建立 map<string, int>
//   方式 B（C++23）：
//     records
//       | views::transform(&LogRecord::user_id)
//       | ...（可用 chunk_by + transform 配合排序，见进阶任务 2）
//   至少用一次 ranges::to<std::map<std::string,int>> 或等价收束
// ============================================================

// ============================================================
// TODO [必做] 5: 按时间戳排序（projection + ranges::sort）
//   收束为 vector<LogRecord> 后排序：
//   std::ranges::sort(records, std::less{}, &LogRecord::timestamp);
// ============================================================

// ============================================================
// TODO [必做] 6: 前 N 条 ERROR 记录（ranges::to 收束）
//   records
//     | views::filter([](const LogRecord& r){ return r.level == Level::ERROR; })
//     | views::take(3)
//     | ranges::to<std::vector<LogRecord>>()
// ============================================================

// ============================================================
// TODO [必做] 7: 输出最终报告
//   打印：
//     总条数
//     INFO / WARN / ERROR 各计数
//     按 user_id 频次表
//     前 3 条 ERROR 的完整内容
// ============================================================

// ============================================================
// TODO [必做] 8: 附对象关系图说明（代码末尾注释块）
//   覆盖以下层的 iterator_concept 和是否 borrowed：
//     - 底层 raw_lines（ref_view 或 span）
//     - 经过 filter 之后
//     - 经过 transform 之后
//     - ranges::to 之后（已脱离 view，成为具体容器）
// ============================================================

// ============================================================
// TODO [进阶] 1: 把输入改成 std::generator（C++23）
//   std::generator<std::string_view> line_producer(
//       std::span<const std::string_view> src) {
//       for (auto& sv : src) co_yield sv;
//   }
//   观察 generator 的 iterator_concept（input_iterator，move-only）
//   与 istream_view 的类比关系（单遍迭代语义）
// ============================================================

// ============================================================
// TODO [进阶] 2: 用 chunk_by 替换手动 unordered_map 分组（C++23）
//   先对记录按 user_id 排序，再：
//   records | views::chunk_by([](const LogRecord& a, const LogRecord& b){
//       return a.user_id == b.user_id;
//   })
//   注意：chunk_by 需要相邻比较，必须先排序
// ============================================================

// ============================================================
// TODO [进阶] 3: 加一层 zip 把记录和行号配对（C++23）
//   auto numbered = std::views::zip(std::views::iota(1), parsed_records);
//   验证 zip_view 的 iterator_concept（random_access）vs
//   iterator_category（input，proxy reference 导致双轨差异）
// ============================================================

// ============================================================
// TODO [进阶] 4: 写一份 1 页小结（注释形式）
//   回答：
//     a) filter_view 的 begin() 缓存在哪里最可能导致意外？
//     b) 哪一处最体现惰性的价值（省去了哪些中间容器）？
//     c) 如果把输入换成 std::generator，哪些地方的 API 调用需要改变？
// ============================================================

// ---- 辅助：把 sample_log 分割为行 ----
// （学员可替换为 istream_view 或 std::generator 版本）
std::vector<std::string_view> split_lines(std::string_view text) {
    std::vector<std::string_view> lines;
    while (!text.empty()) {
        auto pos = text.find('\n');
        if (pos == std::string_view::npos) {
            lines.push_back(text);
            break;
        }
        if (pos > 0) lines.push_back(text.substr(0, pos));
        text.remove_prefix(pos + 1);
    }
    return lines;
}

int main() {
    // 默认骨架输出——填完 TODO 后替换为真实管道
    std::cout << "CAPSTONE1 skeleton — fill TODOs to build pipeline\n";

    // 起点：把 sample_log 分割为行
    // auto raw_lines = split_lines(sample_log);

    // 解析阶段（TODO 2）
    // auto parsed = raw_lines
    //     | std::views::transform(parse_line)
    //     | std::views::filter([](const auto& o){ return o.has_value(); })
    //     | std::views::transform([](const auto& o){ return *o; });

    // 收束为可排序容器（TODO 5）
    // auto records = parsed | std::ranges::to<std::vector<LogRecord>>();

    // 排序（TODO 5）
    // std::ranges::sort(records, std::less{}, &LogRecord::timestamp);

    // 统计（TODO 3）
    // auto info_count = std::ranges::count_if(records,
    //     [](Level l){ return l == Level::INFO; }, &LogRecord::level);

    // 前 3 条 ERROR（TODO 6）
    // auto top_errors = records
    //     | std::views::filter([](const LogRecord& r){ return r.level == Level::ERROR; })
    //     | std::views::take(3)
    //     | std::ranges::to<std::vector<LogRecord>>();

    return 0;
}

// ============================================================
// 对象关系图（TODO 8 填写此处）
// ============================================================
//
// 层                          iterator_concept        borrowed_range
// ─────────────────────────────────────────────────────────────────
// raw_lines (vector<sv>)      contiguous_iterator     NO (owning)
// ref_view(raw_lines)         contiguous_iterator     YES
// | transform(parse_line)     random_access_iterator  NO
// | filter(has_value)         bidirectional_iterator  NO  <-- begin() 非 const
// | transform(unwrap)         bidirectional_iterator  NO
// ranges::to<vector>()        N/A (具体容器，脱离 view)
//
// 触发迭代的位置：
//   ranges::to / ranges::sort / ranges::count_if / for-range
//   在此之前所有 view 均未迭代（惰性）
