# 17 Qt Quick 桥接

这一章把 MediaWorkbench 的 C++ 状态接到 Qt Quick。目标不是重写一个 QML 版应用，而是证明同一套 `MediaListModel` 和 `SessionController` 可以驱动 Widgets 与 Quick 两个界面。

项目入口在 `exercises/U01_quick_bridge`。主 target 是 `c16_quick`。练习四变体 target 是 `c16_u01_reference`、`c16_u01_good`、`c16_u01_bad_rejected` 和未默认运行的 `c16_u01_student`。

## 1. 为什么不能重写业务模型

如果 Quick 另写一套列表、过滤和选择逻辑，两个 UI 会很快分叉：Widgets 保存的是 ID，Quick 保存的是 row；Widgets 会 generation 拒旧，Quick 忘了；Widgets 错误状态能恢复，Quick 只显示成功。这不是桥接，是重复系统。

本章的规则是：QML 只做视图和交互，业务状态仍在 C++。

```cpp
engine.setInitialProperties({
    {"controller", QVariant::fromValue(&controller)},
    {"mediaModel", QVariant::fromValue(controller.model())}
});
```

U01 的界面在 `exercises/U01_quick_bridge/Main.qml`，通过 `quick_bridge.qrc` 编入可执行文件；部署时仍可用 `windeployqt --qmldir C16_Desktop_Multimedia/exercises/U01_quick_bridge` 扫描真实 QML 文件。QML 根对象使用 required properties：

```qml
ApplicationWindow {
    visible: true
    required property var controller
    required property var mediaModel
}
```

这样依赖在创建时显式注入。少传一个对象，QML 加载就失败，而不是运行到某个按钮才发现全局变量为空。

## 2. ListView 与角色

`MediaListModel::roleNames()` 给 QML 角色名：

- `displayName`
- `mediaId`
- `path`
- `error`
- `durationMs`
- `peak`
- `selected`
- `row`

delegate 用 required property 接收角色：

```qml
delegate: ItemDelegate {
    required property string displayName
    required property int row
    text: displayName
    onClicked: controller.selectRow(row)
}
```

`row` 是当前 filtered model 的可见 row，不是永久身份。永久身份仍是 `mediaId`。如果 QML 想长期保存选择，保存 ID；如果只是处理点击，用 row 调 controller。

## 3. 绑定什么时候会断

QML 的文本可以绑定：

```qml
Label { text: controller.selectedName }
```

这要求 C++ 暴露 `Q_PROPERTY(QString selectedName READ selectedName NOTIFY stateChanged)`，并在选择或分析状态变化后 emit `stateChanged()`。如果 C++ 只提供普通成员函数，QML 初次能读到值，但不知道何时刷新。

模型角色也一样。修改 item 后要 emit `dataChanged`，过滤改变要 reset 或发出对应 rows 变化。否则 ListView 留着旧 delegate，看起来像 QML 问题，根因其实是模型通知没发。

## 4. 对象归属和线程

`SessionController` 和 `MediaListModel` 属于 GUI 线程。QML 也在 GUI 线程访问它们。渲染线程只处理 scene graph 渲染，不允许直接修改 C++ model。耗时分析仍走 P1 的 `AnalysisWorker` 线程，结果回到 controller 所在线程后才更新 model。

对象归属规则：

- controller 由 C++ main 持有。
- model 是 controller 的成员，QML 只借用。
- QML root 由 `QQmlApplicationEngine` 持有。
- worker 仍由 controller 安全关闭。

这就是为什么 U01 直接复用 `../P1_media_workbench/app/workbench.cpp`，而不是复制 `MediaItem` 或另写 Quick controller。

## 5. 可运行入口

单独构建 U01：

```powershell
cmake -S C16_Desktop_Multimedia/exercises/U01_quick_bridge -B C16_Desktop_Multimedia/build/u01 -G "NMake Makefiles" -DCMAKE_DEPENDS_USE_COMPILER=FALSE -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="D:/Qt/6.9.2/6.9.2/msvc2022_64"
cmake --build C16_Desktop_Multimedia/build/u01
ctest --test-dir C16_Desktop_Multimedia/build/u01 --output-on-failure
```

`c16_quick --self-check` 会：

1. 用同一 controller 导入 WAV/AVI 夹具。
2. 从 `qrc:/C16QuickBridge/Main.qml` 加载真实 `ApplicationWindow`，并显式注入 controller/model。
3. 通过 C++ model 设置过滤，检查 QML 共享的是同一个 row count。
4. 通过 controller 选择第一项，检查 QML 的 `selectedText` 绑定看到 `selectedName`。

这个 smoke 验证模型通知和绑定路径。它不验证每个像素的视觉布局，也不替代 P1 的 Widgets 视频/audio smoke。

## 6. U01 练习

学生编辑 `student/solution.hpp` 的 `u01::BridgeState`。它是 Quick 桥接的最小状态模型：

1. `add(name)` 增加行。
2. `setFilter(text)` 改变可见行。
3. `select(row)` 使用可见 row 选择真实项。
4. `selectedName()` 是 QML 绑定的数据源。

Reference 和 good 独立实现；bad 忽略过滤并返回常量选择。检查器实际调用所选 `solution.hpp`，bad 必须被拒绝。完整 demo target `c16_quick` 复用 P1 C++，Student 练习不 include Reference。

## 7. 源码阅读入口

阅读 Qt 6.9.2：

- `QQmlApplicationEngine::setInitialProperties`：理解 required property 注入发生在对象创建阶段。
- `QAbstractItemModel` 到 QML ListView 的 role 映射：理解 `roleNames` 和 `dataChanged`。
- Qt Quick scene graph 文档和实现入口：区分 GUI 线程对象访问与 render thread 绘制。
- `QObject` ownership：理解 C++ owner、QML engine owner、parent/child 和裸指针借用的差别。

桥接的核心结论很朴素：UI 可以换技术栈，状态契约不能换。Widgets 和 Quick 共用一套 C++ 模型，才有同一套取消、拒旧、错误和会话语义。
