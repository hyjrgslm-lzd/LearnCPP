# L14 bounded UTF-8 field

目标：实现一个小字段读取器，不读完整 Manifest。字段格式固定为 `u32be length` 后跟 `length` 个 UTF-8 字节。完整推导见 [14：从四个长度字节到一个可信文本字段](../../chapters/14-bounded-binary-fields.md)。

Part 1：用局部 `probe` 读取长度和 payload。只有长度、边界、字符串构造和 UTF-8 校验全部成功，才提交外部 `cursor`。

Part 2：长度大于 `c05::max_text_bytes` 立即返回 `limit_exceeded`。不要按攻击者给的长度先分配。

Part 3：payload 必须严格 UTF-8。错误 offset 要从字段内位置映射回整个输入 buffer 的 byte 坐标。NUL 在通用转码里合法，本题保留它。

Part 4：比较 `validation/bad`。它保留长度、访问边界与 UTF-8 校验，只故意在检查剩余 payload 前提交长度头后的 cursor。截断输入失败时 cursor 错误地变为 4；检查器通过 `truncated payload keeps cursor` 拒绝它。正确版本仅在全部成功后提交局部 probe，详见[样章正文](../../chapters/14-bounded-binary-fields.md)。

Part 5：覆盖边界：非零 cursor、空串、尾随外层字节、每个截断点、过大长度、非法 UTF-8、cursor 越过输入和 `SIZE_MAX` cursor。

检查目标：`L14_binary_fields_reference`、`L14_binary_fields_validation_good`、`L14_binary_fields_validation_bad_rejected`；开启 student preset 后还会运行学生占位。

Reference 在 `src/reference/bounded_field.hpp`，公共实现为 `c05/bytes.hpp`。学生只编辑 `src/student/bounded_field.hpp`。解析重点：用局部 `probe` 让失败路径和分配异常都不推进外部 cursor；只有全部检查成功才提交。
