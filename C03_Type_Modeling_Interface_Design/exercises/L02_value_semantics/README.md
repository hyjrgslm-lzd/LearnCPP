# L02：值语义

先读 `../../chapters/02-value-semantics.md`。你只编辑 `src/student/money.hpp`。

实现 `l02::Money`。金额用 cents 表示，币种用字符串表示。复制得到相同值但独立对象；同币种可相加；异币种必须拒绝；失败后输入保持不变。相等、排序和 `hash_key()` 都必须按同一个值语义使用 cents 与 currency。

解析：值语义看公开值，不看对象地址。`Money{100, "CNY"} + Money{50, "USD"}` 没有定义，不能静默得到某个币种。若相等忽略 currency，但排序或 hash 又包含 currency，容器和缓存会得到互相矛盾的 key 规则。坏变体忽略币种，checker 会拒绝。
