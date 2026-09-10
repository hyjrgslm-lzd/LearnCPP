# 11：日历、UTC、本地时间与时区数据库

10章把“时间点”和“测量间隔”分开了。本章继续把时间点、本地日历字段和时区规则分开。资源清单的存储层只保存 UTC Unix 毫秒；用户报告可以显示成 `America/New_York` 或 `Asia/Shanghai`，但这个选择不能改变包里的时间点。

## 1. 日历对象可以表示非法日期

`std::chrono::year_month_day` 是一个字段组合。`2023y/February/29` 可以构造，但 `ok()` 返回 false。这个设计让解析器能先保存用户输入的年、月、日，再给出诊断，而不是在构造表达式处直接丢信息。

L11 的 UTC 观察题固定检查：

```cpp
static_assert(year_month_day{year{2024}, month{2}, day{29}}.ok());
static_assert(!year_month_day{year{2023}, month{2}, day{29}}.ok());
```

这说明闰年规则属于日历层。它不说明任何时区，也不说明某个本地时刻存在。

## 2. sys_time是UTC时间线上的点

`sys_time<milliseconds>` 是 `system_clock` 时间线上的毫秒精度时间点。`floor<days>(tp)` 得到包含这个 UTC 时间点的 UTC 日期，`year_month_day{day}` 再把天数解释成公历字段。

C05 的 `format_timestamp(0, "UTC")` 生成类似：

```text
1970-01-01 00:00:00.000 UTC +00:00
```

UTC 分支不读取本机时区、不改全局 locale、不需要 tzdb。它只依赖 `chrono` 的日历算法，因此适合作为核心路径。函数先拒绝小于 0001-01-01 或大于 9999-12-31 23:59:59.999 的毫秒值；错误是输入越界，不是格式化失败。

闰秒要单独说明：C05 的字段是 Unix 毫秒，按普通 `sys_time` 处理，不在 wire 中编码闰秒标记。即使某个标准库暴露 leap second 信息，本课 Manifest 也不把 `23:59:60` 作为可写字段。

## 3. local_time没有唯一含义

`local_time` 只是某个时区墙上时钟显示的字段。它还没有绑定时区规则。纽约在 2024 年有两个典型边界：

- `2024-03-10 02:30` 不存在。夏令时跳过这个本地半小时。
- `2024-11-03 01:30` 歧义。夏令时回拨后，这个显示时间出现两次。

`time_zone::get_info(local_time)` 会把它们标成 `nonexistent` 或 `ambiguous`。歧义时间用 `choose::earliest` 和 `choose::latest` 能得到两个不同 UTC 时间点。这个实验的目的不是记住纽约规则，而是看见本地字段不是稳定标识。

MSVC STL 的源码导读会在 17 章展开这里的退出路径：普通 `to_sys(local_time)` 对 nonexistent 和 ambiguous 都抛对应异常；带 `choose` 的重载对 nonexistent 映射到转换边界，对 ambiguous 才用 earliest/latest 选择不同 offset。L11 的检查因此只对 nonexistent 调 `get_info`，只对 ambiguous 调 `to_sys(..., choose::earliest/latest)`。

因此配置里的 `display_zone` 只作为显示参数保存。配置解析阶段只检查它非空；真正 zone 是否存在，必须在显示阶段调用 tzdb，并把失败作为 `DataError` 返回。不能把未知 zone 静默改成 UTC，因为那会生成看似可信但语义错误的报告。

## 4. tzdb能力是独立门槛

C++20 标准有时区数据库接口，但实现和部署会受标准库版本、运行时数据和系统包影响。L11 把 tzdb 检查做成独立测试：普通 UTC 日历检查必须运行，tzdb 不可用时返回 77，被登记为 capability skip。已经运行的断言失败不是 skip；只有库或运行时缺能力才是 skip。

当前 `format_timestamp` 对非 UTC zone 的策略是最小公共策略：

1. `locate_zone(zone)` 找不到，返回 `io_error`。
2. 找到 zone 后，用 UTC 时间点查 offset。
3. 用 `utc + offset` 得到显示字段，输出 zone 名和 offset；历史 offset 若含秒，显示为 `HH:MM:SS`。
4. 本地显示日期若越过 0001-9999，返回 `out_of_range`，不把五位年份或截断年份冒充合法显示。

这里没有实现完整本地输入解析，因为 Manifest 不接收本地时间。若以后需要让用户输入本地日期，必须把 nonexistent/ambiguous 选择写进接口，而不是猜一个默认。

## 5. 练习入口与解析

运行 L11：

```powershell
cmake -S L11_calendar_zones -B build/leaf-L11 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L11 --config Release --parallel 2
ctest --test-dir build/leaf-L11 -C Release --output-on-failure
```

自测问题：

**问：`year_month_day{2023y/2/29}` 能构造，为什么还要 `ok()`？**

答：构造只保存字段，`ok()` 才检查这些字段能否组成真实公历日期。解析器需要这种分离来报告“输入哪一段错了”。

**问：为什么不把配置里的 `display_zone` 在 parse_config 阶段验证存在？**

答：配置解析只处理文本格式和键值规则。时区数据库是运行环境能力；把它放在显示阶段，错误能带上“显示 zone 不可用”的上下文，也不会让读取配置依赖外部数据。

**问：纽约歧义时间为什么影响资源清单？**

答：它说明本地时间字符串不能作为存储格式。清单存 UTC 毫秒后，`2024-11-03 01:30` 的二义性只出现在用户输入或显示选择，不会污染 wire。
