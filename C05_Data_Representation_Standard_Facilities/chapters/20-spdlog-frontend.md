# 20. spdlog同步前端：宏裁剪、运行时过滤与格式后端

本章固定 spdlog v1.17.0 commit `79524ddd08a4ec981b7fea76afd08ee05f83755d`。范围只包含同步 logger 前端、sink、pattern 和格式化错误处理；异步队列、溢出策略、flush/shutdown 与服务观测留给 C08/C11。

## 前端对象：logger 把消息交给 sink

最小可测结构是局部 logger 加可观测 sink：

```cpp
std::ostringstream out;
auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
spdlog::logger logger{"lesson", sink};
logger.info("{}", value);
```

这样测试不污染全局默认 logger，也能直接检查输出文本。`include/spdlog/logger.h` 说明 logger 拥有名称、level 和 sink 列表；formatter 通常由每个 sink 持有。`logger.set_formatter` 会把 formatter 配到各 sink，多 sink 时 clone。`logger::sink_it_` 再按 sink 级别写出。

## 宏裁剪先于运行时过滤

`include/spdlog/spdlog.h` 的 `SPDLOG_LOGGER_DEBUG` 受 `SPDLOG_ACTIVE_LEVEL` 控制。若 active level 高于 debug，宏直接是 `(void)0`，实参不求值：

```cpp
SPDLOG_LOGGER_DEBUG(&logger, "debug {}", side_effect());
```

运行时过滤是另一层。`logger.debug("{}", side_effect())` 或 active debug 宏都会先求值实参，进入 `logger::log_` 后才检查 `should_log`。若 logger level 是 info，debug 消息不会进入 sink，但 `side_effect()` 已执行。

这条边界是日志教学的重点：宏裁剪能移除调用表达式；运行时过滤只能阻止后续格式化和写出。

按源码手推一次 active debug 宏、但 logger level 为 info 的调用：

1. `SPDLOG_LOGGER_DEBUG` 因 active level 允许，展开成 `SPDLOG_LOGGER_CALL`。
2. C++ 调用表达式先求值 `side_effect()`。
3. `logger::debug` 转到 `logger::log(level::debug, fmt, args...)`。
4. `logger::log_` 先算 `log_enabled = should_log(debug)`。
5. `log_enabled` 和 backtrace 都为 false 时直接 return。
6. 因为 return 在 `vformat_to` 前面，消息没有格式化，也没有进 sink。

active level 为 info 时，第一步宏已经是 `(void)0`，后面五步都不存在。

## 格式检查委托给后端

spdlog 的格式接口使用 `format_string_t<Args...>`。当 `SPDLOG_FMT_EXTERNAL=ON` 时，它来自外部 fmt；当 `SPDLOG_USE_STD_FORMAT=ON` 时，它来自标准库。C05 的两个预设分别验证这两条路径，且外部 fmt 路径必须链接本课固定静态 fmt，不能落回 bundled fmt。

自定义 formatter 也跟随后端：fmt 后端特化 `fmt::formatter<T>`，std 后端特化 `std::formatter<T>`。`U03_spdlog` 用同一个 `Reading` 类型验证 `logger.info("{}", reading)` 能穿透到实际后端。若 formatter 自己抛异常，异常先来自格式后端，随后才由 logger 的 catch 分支转给 `error_handler`。

## pattern formatter 的“编译”是运行期配置处理

`logger.set_pattern("[%l] %v")` 会构造 `pattern_formatter`，把 pattern 拆成 flag formatter 列表。这里的 compile 是库内部运行期准备，不是 C++ 常量求值，也不产生类型级格式串检查。pattern 来自配置时仍是运行时文本。

## 异常出口：后端抛出，logger handler 接收

后端格式化可以抛出 `fmt::format_error`、`std::format_error` 或 formatter 自己抛出的异常。`logger::log_` 用 `SPDLOG_LOGGER_CATCH` 捕获并调用 logger 的 `error_handler`。直接调用后端格式化和经由 logger 记录，是两个不同错误通道。

[U03 spdlog](../exercises/U03_spdlog/README.md) 同时验证直接后端错误和 logger error_handler。观察程序还检查 DEBUG/INFO 两个 active level 下的调用次数，避免把运行时过滤误写成宏裁剪。
