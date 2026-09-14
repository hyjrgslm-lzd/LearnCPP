# L02 QObject lifetime

对应正文：[02-qobject-ownership](../../chapters/02-qobject-ownership.md)。

学生只编辑 `student/solution.hpp`。Part：

1. 用 heap parent 创建 `library -> clip` QObject 树。
2. 释放 parent 后 child 必须由 QObject 树销毁。
3. 不接管外部/栈对象；调用者仍拥有它。
4. 用 `deleteLater()` 安排空闲销毁，并让 `QPointer` 能观察到对象失效。

`bad` 故意漏 parent、接管外部对象并同步 delete，检查器覆盖三类错误。
