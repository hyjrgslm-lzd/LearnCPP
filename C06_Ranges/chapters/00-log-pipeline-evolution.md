# 00 日志管道样章：从循环到 ranges 的取舍

这一章只处理一个很小的业务问题：读入若干日志行，过滤非法行，统计级别和用户频次，再取前 3 条 `ERROR`。输入固定为四字段子集：

```text
timestamp,level,user_id,message
```

`level` 只接受 `INFO`、`WARN`、`ERROR`。字段不足、字段过多或级别不识别都算非法行。这里故意不讲完整 CSV：没有引号、转义、换行字段，也不增加 `action` 字段。样章的目标不是造解析库，而是把“正确循环基线、惰性 view 的重复求值、临时对象借用、操作计数和实体化取舍”连起来。

## 先写循环基线

循环版本是本章的正确性标尺。每一行只调用一次 `parse_line`，返回 `std::optional<LogRecord>`；成功才移动进结果容器：

```cpp
std::vector<LogRecord> collect_records_loop(std::span<const std::string_view> lines,
                                            OperationCounts& counts) {
    counts = {};
    std::vector<LogRecord> records;
    records.reserve(lines.size());
    for (std::string_view line : lines) {
        ++counts.parse_attempts;
        if (auto parsed = parse_line(line)) {
            records.push_back(std::move(*parsed));
        }
    }
    return records;
}
```

这段代码没有 ranges，但它给了后面所有版本一个硬契约：同一组 9 行输入，解析尝试必须是 9 次，保留 7 条有效记录，丢弃 `INVALID_LINE_NO_COMMAS` 和 `BADLEVEL`。如果 ranges 版本结果不同，先怀疑 ranges 版本，而不是怀疑循环太土。

`LogRecord` 里的 `timestamp`、`user_id`、`message` 三个文本字段都用 `std::string` 拥有数据，`level` 解析成 `Level` 枚举值。当前样例输入是静态字符串，借用看起来安全；但真实导入常来自文件缓冲、网络缓冲或临时拆分结果。报告对象如果保存 `string_view`，调用者很容易在源缓冲销毁后拿到悬垂引用。这里让文本字段拥有数据，是为了让“日志记录”成为可排序、可存储、可返回的实体。

## 直接管道会重复解析

很多人会自然写出下面的管道：

```cpp
auto records =
    lines
  | std::views::transform(parse_counted)
  | std::views::filter([](const std::optional<LogRecord>& r) { return r.has_value(); })
  | std::views::transform([](std::optional<LogRecord> r) { return std::move(*r); });
```

这段写法的解包 lambda 按值接收 `optional`，所以它不会返回悬垂引用。但它仍然有一个成本问题：`filter_view` 判断当前元素时要解引用上游 `transform_view`，外层 `transform_view` 真正产出 `LogRecord` 时又会解引用一次同一个上游位置。上游的 `parse_counted` 是函数调用，不是缓存字段；每次解引用都会重新解析。

本题样例有 9 行输入、7 行有效。直接管道的解析次数是 `9 + 7 = 16`：每一行先被 `filter` 判断一次，每条有效行再被解包时解析一次。这个结论来自本题 checker 的 `count_reparse_demo`，不是从语法猜出来的。

这就是 ranges 的一个基本代价：view 保存的是“如何取值”，不是“已经取到的值”。惰性可以省掉中间容器，也可能让上游计算被多次触发。上游是廉价成员访问时通常无所谓；上游是解析、查询、锁、I/O、分配或统计时，就必须数清楚。

## 正确 ranges 版本先实体化 optional

为了保持每行只解析一次，本题采用一个很小的实体化点：先把 `optional<LogRecord>` 存入局部 `vector`，再对这个局部容器过滤和解包：

```cpp
auto parsed = lines
  | std::views::transform([&](std::string_view line) {
    ++counts.parse_attempts;
    return parse_line(line);
  })
  | std::ranges::to<std::vector<std::optional<LogRecord>>>();

auto records_view =
    parsed
  | std::views::filter([](const std::optional<LogRecord>& r) { return r.has_value(); })
  | std::views::transform([](std::optional<LogRecord> r) { return std::move(*r); });
```

这里的实体化不是性能优化承诺，只是语义修复：它保证解析结果有稳定存储，后续 filter/transform 重复解引用的是 `optional` 对象本身，不会重复调用 `parse_line`。代价也明确：多一个 `vector<optional<LogRecord>>`，多一次存储和移动。输入很小、解析便宜、只消费一次时，直接管道可能更短；解析贵或计数要求严格时，实体化点更稳。

最后收束使用 `std::ranges::to`：

```cpp
auto records = records_view | std::ranges::to<std::vector<LogRecord>>();
```

本机 MSVC 19.51 已实测支持 `filter | take | std::ranges::to<std::vector<T>>()`。所以本题直接使用这条收束路径；如果以后某个工具链不支持它，应该在该工具链的能力探测和验证记录里单独标 SKIP。

## 临时 optional 解包的反例

另一个常见错误更危险：

```cpp
auto bad =
    lines
  | std::views::filter([](std::string_view line) { return parse_line(line).has_value(); })
  | std::views::transform([](std::string_view line) -> const LogRecord& {
        return *parse_line(line);
    });
```

这个 `transform` 返回的是临时 `optional<LogRecord>` 内部对象的引用。lambda 返回时，临时 `optional` 已销毁，引用立刻悬垂。它可能在小样例里“看起来能打印”，也可能在优化构建、不同标准库或稍大输入下直接坏掉。这个反例不进入自动 bad 目标，因为真实未定义行为不适合当稳定拒绝证据；正文里用它解释根因，checker 里的 bad 则使用稳定的重复解析错误。

根因是对象所有权不在 view 链上。`transform_view` 不会替你延长 lambda 里临时对象的生命期；它只在迭代器解引用时调用函数对象并把结果交给当前表达式。要么返回拥有型值，要么让被引用对象存在于管道之外的稳定存储中。

## 后续汇总

收束出 `std::vector<LogRecord>` 后，排序和统计都使用普通容器语义：

```cpp
std::ranges::sort(records, std::less{}, &LogRecord::timestamp);
auto error_count = std::ranges::count_if(
    records,
    [](Level level) { return level == Level::error; },
    &LogRecord::level);
```

成员指针 `&LogRecord::level` 是 projection。它表达的是“算法先取成员，再把成员交给谓词”。等价 lambda 当然能写，但 projection 在这里少了一层样板，也更清楚地把“取字段”和“判断字段”分开。

按用户计数用 `std::map<std::string, int>` 手动 fold。它比为了演示 `chunk_by` 先排序再分块更直接：我们要的是全局频次，不是相邻分组。`chunk_by` 的价值在“输入已经按 key 排好，想保留分组边界”时出现；本题最终报告只需要频次表，map fold 是更小的正确解。

`summarize(lines, error_limit)` 对 `error_limit` 有公开边界：负数抛出 `std::invalid_argument`，`0` 返回空错误列表，超过匹配数量时只返回全部匹配项。拒绝负数必须发生在调用 `views::take` 前，不能把负数隐式交给 adaptor。

前 3 条错误记录用 `filter + take` 表达，再用 `std::ranges::to` 收束：

```cpp
auto errors =
    records
  | std::views::filter([](const LogRecord& r) { return r.level == Level::error; })
  | std::views::take(3);
auto first_errors = errors | std::ranges::to<std::vector<LogRecord>>();
```

这里的消费端是 `std::ranges::to`。在此之前，`filter` 和 `take` 只保存取值规则。

## View 关系图

本题最关键的对象关系如下：

```text
span<const string_view> lines
  iterator_concept: random_access
  borrowed_range: yes

lines | transform(parse_line)
  iterator_concept: random_access
  borrowed_range: no
  dereference cost: one parse call

... | filter(has_value)
  iterator_concept: bidirectional
  borrowed_range: no
  begin(): non-const, may cache first satisfying iterator

... | transform(unwrap_by_value)
  iterator_concept: bidirectional
  borrowed_range: no
  dereference cost: asks upstream again unless upstream result is stored

vector<optional<LogRecord>> parsed
  concrete storage, owns parsed optional records

vector<LogRecord> records
  concrete storage, owns valid log records
```

`filter_view` 是降级点。底层 `span` 是 random access，但过滤后无法用下标 O(1) 找到“第 n 个满足谓词的元素”，所以迭代器上界降到 bidirectional。它还不是 borrowed range；对右值临时管道调用返回迭代器的算法时，标准库需要防止迭代器离开已销毁 view，因此会走 `std::ranges::dangling` 一类的保护路径。

`filter_view::begin()` 非 const 也来自同一个对象模型：第一次找满足谓词的位置可能要写 begin 缓存。`views::as_const` 只改变元素访问的 const 视角，不把一个需要写缓存的 view 变成 const 可迭代对象。

## 练习要求

实现 `exercises/CAPSTONE1_log_pipeline/src/student/log_pipeline.hpp`。不要改 `main.cpp`、`src/reference`、`validation/good` 或 `validation/bad`。

必须完成：

1. `parse_line`：按逗号切出四字段，字段数不等于 4 或 level 非法返回 `std::nullopt`。
2. `collect_records_loop`：循环基线，每行解析一次，返回 7 条有效记录。
3. `collect_records_ranges_once`：使用 ranges 过滤解包，但保持 `parse_attempts == lines.size()`。
4. `summarize`：排序、level 计数、user 计数、前 N 条 ERROR；负数 `error_limit` 抛 `std::invalid_argument`，`0` 和超限都返回合法结果。
5. `count_reparse_demo`：保留直接惰性管道的重复解析现象，样例输入返回 16。

解析：

- 只让 checker 通过不够；如果 `collect_records_ranges_once` 写成直接 `transform/filter/transform`，结果内容也许对，但操作计数会失败。
- 如果 `LogRecord` 保存 `std::string_view`，当前样例可能通过人工观察，但不满足报告对象的拥有语义。
- 如果把 BADLEVEL 行计入用户表，说明过滤发生在统计之后，管道顺序错了。
- 如果 `count_reparse_demo` 返回 9，说明你没有真的保留重复解析观察；这个函数不是生产实现，它是本章的反例实验入口。

本章停止在单线程内存数据。文件 I/O、完整 CSV、时间解析、并行处理和性能加速比都不在样章范围内。
