# 08 解析、`charconv` 与完整消费

解析是信任边界。外部文本进入程序后，第一步不是“能不能读出一个数”，而是“整段输入是否只表达这个数”。`std::from_chars` 的好处正好在这里：它不分配、不抛异常、不读取全局 locale、不跳过空白，并且返回第一个未消费字符的位置。

`from_chars(first, last, value)` 有两个结果：`ec` 和 `ptr`。`ec == invalid_argument` 表示从起点就不能形成目标值；`ec == result_out_of_range` 表示文本形状像数字，但目标类型装不下；`ec == {}` 只表示前缀解析成功，不表示完整消费。`"42ms"` 对整数来说会成功解析出 `42`，`ptr` 指向 `m`。如果协议字段要求纯数字，就必须检查 `ptr == last`。

C05 公共 API 是 `parse_u32`、`parse_i64`、`parse_finite_double`。它们都接收 `std::string_view`，不 trim，十进制，完整消费，错误 offset 使用 byte 坐标，`field` 复制为拥有型字符串。空串、前导空格、坏符号返回 `invalid_number`；范围超出返回 `out_of_range`；有未消费尾巴返回 `trailing_data`，offset 指向第一个尾巴字节。

浮点还有一个额外约束：模型只接受 finite double。不同实现对 `inf`/`nan` 的接受程度可能不同，但本 API 的名字已经声明了边界，非有限结果不能进入数据模型。能解析的有限值只是“输入有效”；是否需要固定小数位、科学计数法、最短 roundtrip 或十进制精度策略，是格式化或协议层单独决定。

不要用 `std::stoi` 这类接口实现本课解析契约。它们通常会分配临时字符串或抛异常，而且 locale/空白/尾部处理不符合本课边界。也不要先 trim 再解析；配置语法要 trim 时，应在配置解析层明确 trim ASCII 边缘，并把错误位置重新映射回原始输入。

本章练习 `L08_parsing` 要实现三个函数。Reference 直接复用 `include/c05/text.hpp`；`validation/good` 是独立完成体；`validation/bad` 故意只检查 `ec` 而漏掉 `ptr == last`，所以正常数字先通过，再在 `"42x"` 上被 checker 拒绝。Student 初态返回 `not_implemented`，应失败。

自测重点：`u32` 最大值与溢出、`i64` 最小/最大值与溢出、前导空格、符号、尾部字符、浮点 finite、`inf` 和巨大指数。通过本题后，你应该把“解析成功”和“字段完整合法”当成两件必须同时成立的事。
