# 09 格式化、流、locale 与输出边界

格式化把程序值变成人能读的文本。它经常和解析挨在一起，但两者目标不同：解析守住输入边界，格式化控制输出表达。日志、诊断和报告可以追求清晰；机器协议要追求稳定、可逆、和 locale 无关。

`std::format` 返回拥有型 `std::string`，适合一次性构造诊断。`std::format_to` 写入输出迭代器，适合已有缓冲或容器。`std::format_to_n` 最多写入 n 个字符，并返回完整输出长度；这不是“自动成功截断”，而是给调用者一个必须检查的结果。若 `result.size > n`，输出已经不完整，不能当完整字段发布。

动态格式串在 C++23 中应使用 `std::vformat` 和 `std::make_format_args`。坏格式会在运行时抛出 `std::format_error`，所以动态格式来自外部配置时要当输入验证处理。`format_args` 只借用实参；不要把它保存到比实参更长的地方。需要缓存输出时，缓存最终字符串或缓存拥有型参数。

自定义 `std::formatter<T>` 是类型的展示策略。最小 formatter 只需要解析本类型支持的格式规格，再把字段写到 `format_context`。不要为了一个固定输出加工厂、注册表或反射层。若类型同时有机器协议编码和展示输出，formatter 只负责展示；协议编码仍应走显式字段写入和长度检查。

`iostream` 受 locale 影响，适合面向人的输出。例如数字小数点、千分位、日期名称都可能随 locale 改变。机器字段应该使用 `to_chars/from_chars` 或显式指定格式。`std::print`/`std::println` 只是把格式化结果写到 stdout/stderr 的便利设施；它们不是可逆协议。

非 ASCII 文本还有显示宽度问题。UTF-8 字节数、Unicode code point 数、grapheme cluster 数、终端列宽并不相等。`std::format("{:>4}", text)` 的宽度单位不是 UAX #29 字素簇，也不保证东亚宽字符或 emoji 在每个终端对齐。需要精确 UI 排版时，必须使用文本布局库或平台控件；C05 只把这个边界讲清楚。

本章自测 `L09_formatting_observation` 覆盖 `format`、`format_to`、`format_to_n`、动态格式错误、自定义 formatter、locale-aware stream 和非 ASCII 宽度边界。负例 `L09_formatting_bad_truncation` 故意忽略 `format_to_n` 的完整长度报告，应被 `format_to_n reports truncation` 拒绝。

本章讲的是标准设施和输出契约。若要看成熟库怎样把这些规则组织成实际 API，后续[第19章 fmt格式库](19-fmt-library.md)固定 fmt 12.1.0，拆开 `format_string`、`fmt::runtime`、`FMT_COMPILE`、参数擦除和自定义 formatter；[第20章 spdlog同步前端](20-spdlog-frontend.md)再看日志库如何把格式后端、宏裁剪、运行时过滤和 pattern 配置分层。那里不替代本章的标准边界，只把同一组规则放进真实库实现验证。
