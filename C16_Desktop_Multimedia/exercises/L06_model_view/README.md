# L06 model view

对应正文：[06-model-view](../../chapters/06-model-view.md)。

学生只编辑 `student/solution.hpp`。实现 `MediaListModel`：

1. 继承 `QAbstractListModel`，正确处理 parent、row、role。
2. `addClip/insertClip` 使用 `beginInsertRows/endInsertRows`，插入位置不能先改数据再通知。
3. `renameClip` 按稳定 id 更新，发出 `dataChanged`，persistent index 仍指向原对象。
4. `moveClip` 使用 `beginMoveRows/endMoveRows`，移动后 selection 经 proxy 映射仍回到同一 id。
5. `removeClip` 使用 `beginRemoveRows/endRemoveRows`，被删除行的 `QPersistentModelIndex` 必须失效。
6. roleNames 覆盖 `id/title/duration`。
7. 通过 `QAbstractItemModelTester::QtTest`、proxy、selection 和身份断言检查。

`bad` 漏结构通知、身份用 row 冒充。检查器使用 QtTest/qExec，结构失败是正常非零退出，不用崩溃冒充反例。
