# 10：时钟、duration与可序列化时间

资源清单里有一个字段叫 `modified_at_ms`。它看起来只是整数，实际跨了三层含义：文件在某个时间点被修改、包格式用 UTC Unix 毫秒保存、程序运行时可能还要测量“编码用了多久”。这三件事不能共用一个时钟概念。

硬先修是 C01 的最小构建、C02 的对象和值存活、C03 的错误返回，以及本课 03 的整数范围。这里会临时使用 `duration<Rep, Period>` 这个类模板：`Rep` 是底层计数类型，`Period` 是一个编译期有理数，表示一个 tick 等于多少秒。它不要求先读 C04 泛型课；本章只用它说明“单位”是类型的一部分。

## 1. wall clock不是秒表

`std::chrono::system_clock` 表示系统 wall clock。它适合和真实日期、文件时间、日志时间互相转换，但机器可以因为 NTP、用户设置或虚拟机恢复而调整它。用它测量“这段代码耗时”会把系统调时混进结果。

`std::chrono::steady_clock` 的承诺是单调，不会在一次进程观察里倒退。它适合测量间隔，却没有可携带的 epoch。你不能把 `steady_clock::now().time_since_epoch().count()` 写进文件，然后期待另一台机器理解这个整数表示哪天哪时。

L10 的观察程序只做有限断言：

```cpp
const auto mono_a = std::chrono::steady_clock::now();
const auto mono_b = std::chrono::steady_clock::now();
check(mono_b >= mono_a, "steady_clock does not step backward inside one process observation");
```

这条检查证明当前进程里的观测符合单调时钟用法；它不证明所有硬件不会漂移，也不证明 `system_clock` 永不调整。结课项目保存时间时只接受 UTC Unix 毫秒；测耗时才用 `steady_clock`。

## 2. duration转换不是改变量名

`2500us` 转成毫秒有多个合法答案，取决于你要什么语义：

- `duration_cast<milliseconds>(2500us)` 得到 `2ms`，它截断小数部分。
- `floor<milliseconds>(2500us)` 也是向下取整。
- `ceil<milliseconds>(2500us)` 得到 `3ms`。
- `round<milliseconds>(2500us)` 按标准的舍入规则处理中点。

如果字段协议写明“毫秒”，就必须先决定亚毫秒输入如何处理。C05 的 Manifest 不从浮点秒猜毫秒；它直接接收整数毫秒，并检查范围。这样不会出现 `0.1` 秒的二进制浮点误差，也不会把截断规则藏在调用点里。

另一个常见错误是先取 `count()` 再换单位。`seconds{1}.count()` 是 1，不是 1000。只有 `duration_cast<milliseconds>(seconds{1}).count()` 才是协议需要的毫秒数。类型里有单位时，尽量让 `chrono` 做单位换算，最后一刻再取整数。

## 3. 范围先于转换

资源清单限定时间戳在公历 0001-9999 的 UTC Unix 毫秒范围：

```text
[-62135596800000, 253402300799999]
```

这个范围小于 `int64_t` 的完整范围，但大于很多中间类型。读取配置、二进制字段或文件时间时，要先问“这个值是否能表达成目标协议的毫秒”，再窄化、加减或显示。

本课公共入口 `c05::format_timestamp` 接受 `int64_t millis`，先检查范围。UTC 格式化只用 `sys_time<milliseconds>`、`floor<days>` 和 `year_month_day`，不依赖系统时区数据库。非 UTC zone 在 11 章处理；如果库或运行环境没有 tzdb，返回 `DataError`，不能静默退回 UTC。

## 4. file_clock桥接的边界

`std::filesystem::last_write_time` 返回 `file_time_type`，它通常基于 `file_clock`，epoch 和精度由实现决定。C++20 提供了时钟转换设施，但不同标准库支持程度不一。课程里不把文件系统时间作为权威输入：P1 的 fixture 构造 typed manifest，外部只读配置和二进制包。

若生产代码必须从真实文件时间生成包字段，转换链应该显式写成：

1. 读 `last_write_time(path, error_code)`，先处理文件系统错误。
2. 把 `file_time_type` 转成 `sys_time`，记录目标库支持方式。
3. 截断或舍入到毫秒，并检查 C05 时间戳范围。

这条链每一步都有失败条件。把 `file_time_type::time_since_epoch().count()` 直接写入 wire 是格式错误，因为那个 epoch 不是协议的一部分。

## 5. 练习入口与解析

运行 L10：

```powershell
cmake -S L10_clocks_durations -B build/leaf-L10 -G "Visual Studio 18 2026" -A x64
cmake --build build/leaf-L10 --config Release --parallel 2
ctest --test-dir build/leaf-L10 -C Release --output-on-failure
```

自测问题：

**问：为什么 Manifest 保存 UTC 毫秒，而不是本地时间字符串？**

答：UTC 毫秒是一个时间点。本地时间字符串依赖时区规则；同一个 `2024-11-03 01:30` 在纽约可以对应两个 UTC 时间点。保存时间点，显示时再选择 zone，歧义才不会进入数据层。

**问：为什么不在 C05 做耗时 benchmark？**

答：本章没有性能优化结论。观察 `duration` 语义不需要 benchmark。若要比较格式化性能，必须固定输入、进程采样、计时区间和归因证据；这里没有提出那种问题。
