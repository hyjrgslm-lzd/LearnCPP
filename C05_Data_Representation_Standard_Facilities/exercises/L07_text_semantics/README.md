# L07 文本语义、规范化与展示策略

观察目标：编码合法性只回答“这串字节能不能解释成 Unicode scalar value 序列”。它不回答“用户看见几个字符”“两个名字是否等价”“大小写折叠后能不能搜索到”。这些属于更高层文本语义。

Part 1：运行 `L07_text_semantics_observation`，比较 `U+00E9` 与 `U+0065 U+0301`。两者都合法，也常被视为规范等价；但 UTF-8 字节不同，code point 数不同。资源清单的 `name/path` 身份规则仍按原始 UTF-8 字节比较，不能因为启用 ICU 就在存储层悄悄 NFC 化、casefold 或 locale 化。

Part 2：规范化有四个常见形式。NFD 做 canonical decomposition，NFC 在 NFD 基础上 recomposition；NFKD 会做 compatibility decomposition，NFKC 再 composition。`é` 的组合形式适合展示和搜索解释，`①` 这类兼容字符进入 NFKC 后可能变成普通数字，已经改变了用户输入的表记。课程策略是：协议和 manifest 保留原字节身份；展示、搜索、去重提示可以在单独索引上规范化，并必须说明成本和丢失的信息。

Part 3：大小写不是 ASCII 的 `tolower`。case mapping 可以依赖 locale，例如土耳其语的 I；casefold 是更适合大小写无关匹配的语言中立折叠，但仍不是身份比较。保存资源路径时不要 casefold；用户搜索框可以 casefold 到派生索引。

Part 4：grapheme cluster 是用户感知的文本边界，可能由多个 code point 组成。ZWJ emoji、组合音标、区域旗帜都不能用“一个 code point 一个字符”切割。没有 ICU 时，本课只展示这个边界；完整 UAX #29 验证在 U01。

负例 `L07_text_semantics_bad_identity` 故意把 canonical equivalence 当成 manifest 身份，应该被 `canonical-equivalence is not byte identity` 精确拒绝。
