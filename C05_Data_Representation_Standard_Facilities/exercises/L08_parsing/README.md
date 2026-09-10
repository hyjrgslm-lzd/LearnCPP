# L08 解析与 `from_chars`

本题实现 `parse_u32`、`parse_i64`、`parse_finite_double`。输入是调用者给出的 `std::string_view`，函数只借用它，返回值拥有自己的数字或 `DataError`。错误里的 `field` 必须复制字段名，不能保存 view。

Part 1：整数解析只接受十进制完整文本。`std::from_chars` 不跳过空白，不使用当前 locale，也不会分配内存；这正适合配置和二进制旁路文本里的机器字段。调用后必须检查 `ptr == last`。如果只检查 `ec`，`"42ms"` 会被当成 `42`，下游单位就丢了。

Part 2：错误分类要能定位输入边界。空串、前导空格、符号不合法等从第 0 字节失败，返回 `invalid_number`。范围超过目标类型返回 `out_of_range`。成功解析了一段但没有消费全部输入时返回 `trailing_data`，offset 是第一个未消费字节。

Part 3：浮点解析仍然不是“随便能表示就行”。课程 API 名为 `parse_finite_double`，所以 `inf`、`nan` 或溢出结果不能进入模型。可解析的有限数值返回成功；非有限值返回 `invalid_value` 或范围错误。格式化回写和 roundtrip 是协议层策略，不在本函数里偷偷完成。

编辑位置：`src/student/text_parse.hpp`。Reference 直接转发公共 `c05/text.hpp`；`validation/good` 是独立完成体；`validation/bad` 故意漏掉完整消费检查，普通数值先通过，再被 `"42x"` 精确拒绝。
