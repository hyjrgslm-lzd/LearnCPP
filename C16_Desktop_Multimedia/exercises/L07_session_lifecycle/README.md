# L07 session lifecycle

对应正文：[07-application-session](../../chapters/07-application-session.md)。

学生只编辑 `student/solution.hpp`。实现 `SessionStore`：

1. 用 `QSaveFile` 保存 JSON。
2. 保存版本、最近媒体、窗口 geometry、DPI scale、locale。
3. commit 前失败必须保留旧文件。
4. 无文件、坏 JSON、未知版本回退到默认会话。
5. `lastError()` 保留最近一次保存失败的阶段与错误信息；下一次保存开始时清除旧错误。

检查器先确认临时目录有效，再保存并读取错误。短写必须取消临时写入，不能通过非原子覆盖补救。先保存返回值，再读取 `lastError()`，避免函数实参求值顺序让诊断读到上一次错误。

`bad` 使用 `QFile::Truncate`，在失败路径破坏旧文件，并漏掉多个字段。
