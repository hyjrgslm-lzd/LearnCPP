# L09 格式化、流与 locale

观察目标：格式化是把程序值变成展示文本；解析是把外部文本变成程序值。它们可以互相配合，但不能自动保证机器协议可逆。

Part 1：`std::format` 返回拥有型 `std::string`，适合构造诊断、日志和报告。`std::format_to` 把输出写入迭代器，适合复用外部缓冲。自定义 `std::formatter<T>` 是类型自己的展示策略，最小实现只需要 `parse` 和 `format`；不要为了一个固定展示再造一套格式化框架。

Part 2：`std::format_to_n` 是有界写入工具。它最多写 `n` 个字符，同时返回完整输出长度。只看“没有越界”不够；当 `result.size > n`，文本已被截断，不能当完整协议字段发布。负例 `L09_formatting_bad_truncation` 故意忽略这个事实，应被精确拒绝。

Part 3：动态格式串在 C++23 中应通过 `std::vformat` 和 `std::make_format_args` 处理，坏格式会在运行时抛出 `std::format_error`。`format_args` 持有对实参的借用，不要把它保存到比实参更长的生命周期；需要长期保存时保存值本身或保存最终字符串。

Part 4：`std::print`/`std::println` 是输出便利设施，不是机器可逆协议。`iostream` 受 locale 影响，适合面向人类的展示；机器字段用 `to_chars/from_chars` 或显式格式。非 ASCII 文本的字节数、code point 数、grapheme cluster 数和终端列宽也不是一回事，格式化库不会替你完成完整排版语义。

编辑位置：无，本题是观察题和负例控制题。
