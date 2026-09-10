# L11 calendars and time zones

目标：把日历合法性、UTC时间点、本地时间和时区数据库分开。编辑入口无；本题是观察型。

Part 1：用 `year_month_day::ok()` 判断日历日期。`2024-02-29` 合法，`2023-02-29` 不合法；构造对象本身不等于日期有效。

Part 2：用 `sys_time<milliseconds>` 表示资源清单里的 UTC Unix 毫秒。课程限制年份 0001-9999，显示前先检查毫秒范围。

Part 3：独立运行 tzdb 能力检查。`America/New_York` 在 `2024-03-10 02:30` 有 nonexistent local time，在 `2024-11-03 01:30` 有 ambiguous local time；`choose::earliest/latest` 只用于歧义时间并选择不同 UTC 时间点。历史 offset 若含秒，`format_timestamp` 要打印 `HH:MM:SS`；显示后越过 0001-9999 要拒绝。

运行：

```powershell
cmake -S L11_calendar_zones -B build/leaf-L11 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L11 --config Release --parallel 2
ctest --test-dir build/leaf-L11 -C Release --output-on-failure
```

解析：UTC测试必须通过。tzdb测试返回 77 表示当前标准库或运行环境没有可用时区数据库；这只影响本地时区观察，不影响 UTC 毫秒范围、日历合法性和 `format_timestamp(..., "UTC")`。
