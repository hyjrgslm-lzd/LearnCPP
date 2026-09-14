# 06 Model/View：身份、视图与检查器

MediaWorkbench 的媒体列表不能只用 `QStringList` 拼 UI。审阅器需要显示标题、时长、路径、分析状态；排序和过滤后，用户选择的仍是同一个媒体条目，而不是“当前第 3 行”。Qt 的 Model/View 把数据身份、视图呈现、选择和代理分开，核心入口是 `QAbstractItemModel`。P1 会有自己的 `workbench::MediaListModel`，本章不是让 P1 直接 include 练习答案，而是规定它要满足同一类协议：稳定身份、结构通知、proxy 后可追踪选择。

本章练习实现 `QAbstractListModel`。先从 index 契约开始：`QModelIndex` 不是数据本身，它是“模型 + row + column + internal id/pointer”的定位令牌。视图拿 index 问模型要 `data(index, role)`；代理模型会把 proxy index 映射回 source index；selection 保存的是 index 集合而不是业务对象。只要结构变化时通知错了，视图手里的 index 就可能指到错误行。

最小模型也要满足几个不变量：

- `rowCount(parent)` 对有效 parent 返回 0，因为 list model 没有子节点。
- `data(index, role)` 检查 index 有效和 row 范围。
- 插入行必须包在 `beginInsertRows/endInsertRows` 中。准备阶段先验证 row 和输入，确认不会失败后，再调用 begin；begin 之后到 end 之间只做对应结构修改，不能抛出异常或提前返回。
- 删除行必须包在 `beginRemoveRows/endRemoveRows` 中。删除后，指向该行的 `QPersistentModelIndex` 应失效。
- 移动行必须包在 `beginMoveRows/endMoveRows` 中。源行移动到更靠后位置时，Qt 的 destination child 是“移动前坐标系里的插入点”，单行从 0 移到 2 要传 3，修改 vector 后最终仍在 row 2。
- 修改已有行要发 `dataChanged`，并指出变更 roles。
- 业务身份用稳定 id role 表达，不能用 row 冒充。

稳定 id 是 model/view 的关键。排序、过滤、插入都会改变 row。如果 selection 保存 row，proxy 一变就指向别的媒体；如果保存源模型 index 或 id，就能映射回来：

```cpp
QSortFilterProxyModel proxy;
proxy.setSourceModel(&model);
auto proxyIndex = selection.selectedRows().front();
auto sourceIndex = proxy.mapToSource(proxyIndex);
auto id = model.data(sourceIndex, MediaListModel::IdRole).toString();
```

一个演进过程可以这样走。第一版只 append：

```cpp
beginInsertRows({}, row, row);
clips_.push_back(std::move(clip));
endInsertRows();
```

这能让视图知道“尾部多了一行”，但还不能证明任意位置插入、删除、移动正确。第二版加入 `insertClip(row, clip)`，在 begin 前检查 `0 <= row <= rowCount()`，begin/end 中只修改 `clips_`。第三版加入 `moveClip(id, destination)`：先用稳定 id 找 source row，再调用 `beginMoveRows`，最后移动 vector 元素。第四版删除：`beginRemoveRows` 后 erase；原来指向被删行的 `QPersistentModelIndex` 必须变 invalid。每一步都只改变一个结构操作，失败前提也清楚：输入非法时不调用 begin，数据保持不变。

`QPersistentModelIndex` 用来观察“同一个 index 在模型变化后是否仍有效”。它不是万能身份系统；删除对应行后它应失效，移动或重命名时它应继续指向同一业务 id。检查器用它验证 rename 不破坏身份、move 后仍能读到 id `b`、remove 后旧 index 失效。

`QAbstractItemModelTester` 是本课的真实失败模式入口。练习使用 `QAbstractItemModelTester::QtTest` 放在 QtTest/qExec 用例里；模型结构错误会变成普通测试失败和非零退出，不靠崩溃冒充检查通过。它会检查 index parent、row/column、insert/remove/move signal 成对等结构性错误。它不能替你验证业务语义，所以本题还额外检查 id role、title role、proxy filter、selection 映射、删除后 persistent 失效。结构检查加内容断言，才算覆盖 model/view 的两层契约。

练习 [L06_model_view](../exercises/L06_model_view/README.md) 要实现 `MediaListModel`。Part 1 建立 roles 和 append；Part 2 任意位置插入；Part 3 rename 后 persistent index 保持身份；Part 4 move 后 proxy/selection 仍映射到同一 id；Part 5 remove 后 persistent index 失效。Reference/good 都用真实 begin/end 通知；bad 版本漏结构通知、用 row 当 id，QtTest 与身份断言会拒绝它。L08 accessibility 要求视图控件暴露可访问名称和键盘选择路径；P1 的本地审阅列表要反向满足本章同一套协议。
