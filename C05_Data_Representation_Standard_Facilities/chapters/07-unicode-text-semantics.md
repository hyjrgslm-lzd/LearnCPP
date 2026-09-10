# 07 Unicode 文本语义与 ICU 边界

L05 和 L06 解决了两件底层问题：一个 Unicode scalar value 如何落到 UTF-8/UTF-16 code unit 上，以及怎样拒绝非法字节序列。工程里还会遇到另一类问题：用户说“这是同一个字符”“这个名字大小写相同”“光标向右移动一个字符”，这些都不是编码合法性问题。它们属于文本语义。

最常见的误判来自 `é`。`U+00E9` 是一个 code point；`U+0065 U+0301` 是两个 code point，显示上经常也是 `é`。两者在 Unicode 里 canonical equivalent，但 UTF-8 字节不同。C05 的资源清单把 `name` 和 `path` 当存储身份字段：身份按原始 UTF-8 字节比较。启用 ICU 后可以为搜索或展示建立派生索引，但不能在保存 manifest 时偷偷把字段规范化。否则旧包重读再写会改变字节，签名、增量同步和人工 diff 都会失真。

规范化的四个常见形式分别服务不同目的。NFD 把 canonical composition 拆开，NFC 通常把 canonical 序列组合回稳定形式。NFKD/NFKC 进一步处理 compatibility character，例如圈号数字、全角形式、某些排版形式。compatibility 规范化更适合搜索召回或安全审查，不适合无提示改写用户输入，因为它可能丢掉“这个字符原本怎样写”的信息。

大小写也要分层。ASCII 的 `tolower` 只覆盖很小的一段。Unicode case mapping 可能依赖 locale，土耳其语的 `I` 是典型例子；casefold 更像语言中立的大小写无关匹配转换，适合搜索键，但仍然不是存储身份。资源路径尤其不能自动 casefold，因为不同文件系统的大小写语义不一致，本课路径字段又只是元数据，不打开真实文件。

grapheme cluster 是用户感知的边界。一个 emoji family 可能由多个 code point 和 ZWJ 组成；一个带音调的字母也可能由 base code point 加 combining mark 组成。按 code point 截断会切坏显示文本；按 UTF-16 code unit 截断还可能切开 surrogate pair。没有 ICU 时，本课只让你观察这些层次；U01 使用 ICU BreakIterator 和 Unicode 16.0 官方 GraphemeBreakTest 验证默认边界。

本章自测 `L07_text_semantics_observation` 验证三条事实：规范等价不等于字节身份；显示簇可以包含多个 code point；UTF-16 offset 和 UTF-8 byte offset 是不同坐标。负例 `L07_text_semantics_bad_identity` 故意把 canonical equivalence 当 manifest 身份，应被拒绝。

源码入口：`exercises/L07_text_semantics/observation.cpp` 和 `exercises/U01_icu_unicode/checks/icu_unicode_checks.cpp`。退出路径：观察题只证明概念边界；真正规范化、casefold 和 grapheme 的库级能力由 U01 在 ICU 77.1 / Unicode 16.0 上验证。
