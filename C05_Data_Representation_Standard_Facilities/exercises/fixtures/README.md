# 独立黄金输入

golden.hpp的octets由协议布局手写，不调用被测writer、read_be或append_be生成。它们为版本练习和综合项目提供独立oracle。变更格式时必须重新解释布局并审查，不能看到writer输出不同就自动“更新快照”。

v1表示一条id=1、name="A"、path="a"、byte_size=0、modified_at_ms=0（1970-01-01T00:00:00.000Z）、note缺失的资源。

| 偏移 | 字段 | 字节数与值 |
|---|---|---|
| 0 | magic | 4，43 30 35 4D，即ASCII C05M |
| 4 / 6 | major / minor | 各2，大端1 / 1 |
| 8 | record_count | 4，大端1 |
| 12 | body_length | 4，大端52=0x34 |
| 16 | tag1 / len / id | 2+4+4，id为1 |
| 26 | tag2 / len / name | 2+4+1，payload为41 |
| 33 | tag3 / len / path | 2+4+1，payload为61 |
| 40 | tag4 / len / byte_size | 2+4+8，payload全零 |
| 54 | tag5 / len / timestamp | 2+4+8，payload全零 |

总长12+4+52=68。v2_empty_note将minor改为2，在偏移68追加tag6与零长度，所以body_length=58、总长74。空字符串备注与备注缺失是不同状态。v1 reader读取v2后有意丢失未知备注，不能据此声称重编码无损。

检查器还必须验证非黄金输入、错误输入及不同字段顺序；这两份oracle不覆盖全部协议。
