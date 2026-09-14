# 07 应用会话、版本与原子保存

桌面应用关闭时通常要保存会话：最近打开的媒体、窗口位置、分隔条、当前布局、DPI、语言环境。MediaWorkbench 的 session 不是业务数据库，它是“下次启动能恢复用户上下文”的小文件。小文件也要认真处理失败路径，因为写坏 session 会让应用下次启动进入错误状态。

本课使用 JSON 加 `QSaveFile`。`QSaveFile` 先写临时文件，`commit()` 成功后再替换目标文件。这样进程在写入中途失败时，旧文件仍在。直接 `QFile(path).open(WriteOnly | Truncate)` 的问题是打开那一刻旧文件就没了，后面 JSON 生成或写入失败会留下空文件。

会话文件必须有版本：

```json
{
  "version": 1,
  "recentMedia": "clip-a",
  "geometryBase64": "...",
  "devicePixelRatio": 1.5,
  "locale": "zh_CN"
}
```

版本不是为了显得正规，而是给未来迁移一个分支点。没有版本时，新代码无法区分“旧格式”和“损坏文件”。读取失败、JSON 非对象、未知版本，都应回退默认状态；这不是静默吞错，而是把配置恢复和用户数据区分开。真实产品可以记录日志，但本课不把本机日志写进教材。

窗口 geometry 是 `QByteArray`，保存成 base64 字符串。DPI 和 locale 不能当作装饰字段。DPI 变化时，同样的 byte geometry 可能来自另一块屏幕或另一套缩放；locale 变化时，按钮文本、菜单宽度和 layout direction 都可能变。最小观察入口是：保存 `devicePixelRatio=1.5` 与 `locale=zh_CN`，读取后先判断当前环境是否仍匹配；匹配时恢复 geometry，不匹配时可以只恢复最近媒体和用户可确认的布局选择。练习只检查字段往返和未知版本回退，不伪造多屏环境。

恢复顺序也有因果。先创建 `QApplication`，让 Qt 读取平台、字体、screen 和翻译环境；再创建主窗口；然后读取 session；最后决定是否 `restoreGeometry()`。如果在 application 之前读取并应用窗口字节，代码没有可用的屏幕/DPI 事实，只是在把旧机器状态硬塞进当前环境。若 session 里记录的 locale 和当前 locale 不同，MediaWorkbench 可以保留最近媒体，但让布局重新计算，避免旧语言下的窄按钮遮住新语言文本。

保存失败的正确行为是“报告失败并保留旧文件”。检查器会先写入旧 session，再模拟 commit 前失败，确认文件字节没有变。这里的模拟失败是教学入口，不伪造磁盘错误或权限错误；它只验证我们的代码是否在 commit 前有保留旧文件的路径。

读取失败也不能“捡能读的字段”。坏 JSON 直接回默认状态；未知版本同样回默认状态，不能看到 `recentMedia` 就采用、看到 `geometryBase64` 就恢复。部分采用的问题是读者很难知道哪些字段来自未来语义：version 99 的 `geometryBase64` 可能已经换了坐标系，`devicePixelRatio` 可能表达逻辑 DPI 而不是 scale factor。没有迁移代码时，整份未来 session 都应拒绝。

练习 [L07_session_lifecycle](../exercises/L07_session_lifecycle/README.md) 实现 `SessionStore`。Reference/good 使用 `QSaveFile`，保存 version、recent media、geometry、DPI、locale；bad 用 `QFile::Truncate`，在失败路径破坏旧文件，并会部分采用未知版本字段。下游 P1 会把同类协议接到 MediaWorkbench 启动/关闭流程，L01 的 application 生命周期和 L04 的分发顺序是它的前置。
