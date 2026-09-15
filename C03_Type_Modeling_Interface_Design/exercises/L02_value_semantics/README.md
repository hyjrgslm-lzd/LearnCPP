# L02：值语义

先读 `../../chapters/02-value-semantics.md`。你只编辑 `src/student/money.hpp`。

实现 `l02::Money`。金额用 cents 表示，币种用字符串表示。复制得到相同值但独立对象；同币种可相加；异币种必须拒绝；失败后输入保持不变。相等、排序和 `hash_key()` 都必须按同一个值语义使用 cents 与 currency。

解析：值语义看公开值，不看对象地址。`Money{100, "CNY"} + Money{50, "USD"}` 没有定义，不能静默得到某个币种。若相等忽略 currency，但排序或 hash 又包含 currency，容器和缓存会得到互相矛盾的 key 规则。坏变体忽略币种，checker 会拒绝。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L02_value_semantics_student`；默认学生测试关闭时仍生成该项目，但它是 `EXCLUDE_FROM_ALL`，需显式构建。
学生只编辑：`src/student/money.hpp`。 `checks/`、`validation/`、`src/reference/`、diagnostic、support 目标是只读对照/验证/实验入口，保留在题目分组内。
修改 Student 后先重新构建对应目标，再运行 CTest 或 README 中列出的检查命令。
