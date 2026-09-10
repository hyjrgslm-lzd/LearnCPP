# CAPSTONE1：日志管道实现题

先读 `../../chapters/00-log-pipeline-evolution.md`。本题只编辑 `src/student/log_pipeline.hpp`。

## 目标

实现一个四字段日志流水线：

```text
timestamp,level,user_id,message
```

保留字段只有 `timestamp`、`level`、`user_id`、`message`。非法行包括字段数不是 4 或 `level` 不是 `INFO` / `WARN` / `ERROR`，必须被过滤。不要增加 `action` 字段，不要把题目扩成完整 CSV 解析器。

## 给定文件

- `include/log_pipeline_data.hpp`：本题共享输入。Reference、good、bad 和 checker 都用这组数据。
- `main.cpp`：共享 checker。它按 include 路径消费当前实现，不包含答案。
- `src/student/log_pipeline.hpp`：学生待完成实现，初态安全返回空结果，会被行为检查拒绝。
- `src/reference/log_pipeline.hpp`：作者参考实现。
- `validation/good/log_pipeline.hpp`：独立正确完成体，不 include Reference。
- `validation/bad/log_pipeline.hpp`：真实错误算法。它用 `transform(parse)->filter(optional)->transform(unwrap)` 直接消费，有效行会重复解析。

## 需要实现的接口

```cpp
namespace capstone1 {
enum class Level { info, warn, error };

struct LogRecord {
    std::string timestamp;
    Level level;
    std::string user_id;
    std::string message;
};

struct OperationCounts {
    int parse_attempts;
};

struct PipelineReport {
    std::vector<LogRecord> records;
    std::map<Level, int> level_counts;
    std::map<std::string, int> user_counts;
    std::vector<LogRecord> first_errors;
    OperationCounts counts;
};

std::optional<LogRecord> parse_line(std::string_view line);
std::vector<LogRecord> collect_records_loop(std::span<const std::string_view> lines,
                                            OperationCounts& counts);
std::vector<LogRecord> collect_records_loop(std::span<const std::string_view> lines);
std::vector<LogRecord> collect_records_ranges_once(std::span<const std::string_view> lines,
                                                   OperationCounts& counts);
std::vector<LogRecord> collect_records_ranges_once(std::span<const std::string_view> lines);
PipelineReport summarize(std::span<const std::string_view> lines, int error_limit);
OperationCounts count_reparse_demo(std::span<const std::string_view> lines);
}
```

不带 `OperationCounts&` 的 overload 给 B01 计时切片复用同一算法但关闭计数；本题 checker 仍使用带计数版本判定 `parse_attempts`。

## 验收点

checker 会验证：

- 合法行解析为拥有型 `std::string` 字段，非法行返回 `std::nullopt`。
- 显式循环基线解析 9 行、保留 7 条有效记录。
- 正确 ranges 版本和循环基线结果一致，且 `parse_attempts == 9`。
- `summarize` 给出 `INFO=2`、`WARN=2`、`ERROR=3`，用户计数为 `alice=3`、`bob=2`、`carol=2`，前 3 条 ERROR 顺序正确。
- `summarize(lines, -1)` 抛出 `std::invalid_argument`，`summarize(lines, 0)` 返回空错误列表，超限只返回全部匹配错误。
- `count_reparse_demo` 返回 `parse_attempts == 16`，用于证明安全按值管道仍会在有效行上重复解析。
- `std::span` 输入是 borrowed，接 `filter` 后不是 borrowed，且迭代器从 random access 降为 bidirectional。

`validation/bad` 的精确拒绝文本是：

```text
valid records must parse each input line once
```

## 实现提示

先写循环基线。它是正确性标准：每行调用一次 `parse_line`，成功才移动进 `std::vector<LogRecord>`。

随后再写 ranges 版本。本题要求用 `std::ranges::to` 完成实体化与最终收束。不要直接写：

```cpp
lines | views::transform(parse)
      | views::filter(has_value)
      | views::transform(unwrap)
```

这个写法不会悬垂，只要解包按值返回；但它会在有效行上重复 dereference 上游 `transform_view`，从而重复解析。最小修复是先把 `optional<LogRecord>` 结果实体化到一个局部 `vector`，再对这个 `vector` 过滤和解包。
