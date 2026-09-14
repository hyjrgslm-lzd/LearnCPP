# U01 Quick Bridge

对应正文：`../../chapters/17-quick-bridge.md`。

## 你要完成什么

学生练习只编辑 `student/solution.hpp` 的 `u01::BridgeState`。它是 Quick 桥接的最小状态模型：

- `add(name)` 增加行。
- `setFilter(text)` 改变可见行。
- `select(row)` 使用可见 row 选择真实项。
- `selectedName()` 作为 QML 绑定的数据源。

完整 Quick demo target 是 `c16_quick`。它不重写业务模型，而是直接复用 P1 的 `workbench::SessionController` 和 `MediaListModel`。

## 已提供

- `student/solution.hpp`：安全占位，默认失败。
- `reference/solution.hpp`：完整参考实现。
- `good/solution.hpp`：独立正确实现。
- `bad/solution.hpp`：忽略过滤并返回常量选择。
- `checks.cpp`：验证学生状态模型。
- `quick_main.cpp`：加载资源化 QML，required properties 注入 controller/model。
- `Main.qml` 与 `quick_bridge.qrc`：真实 `ApplicationWindow` 入口，便于阅读和 `windeployqt --qmldir` 扫描。

## 检查什么

四变体检查验证状态作业。`c16_quick_smoke` 额外验证：

- QML root 是可见 `ApplicationWindow`。
- `ListView.count` 跟随同一个 C++ model 的过滤结果。
- `controller.selectRow()` 后 QML `selectedText` 绑定看到 `selectedName`。
- QTest 点击 Quick play button，调用共享 `SessionController`。

## 命令

```powershell
cmake -S C16_Desktop_Multimedia/exercises/U01_quick_bridge -B C16_Desktop_Multimedia/build/u01 -G "NMake Makefiles" -DCMAKE_DEPENDS_USE_COMPILER=FALSE -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="D:/Qt/6.9.2/6.9.2/msvc2022_64"
cmake --build C16_Desktop_Multimedia/build/u01
ctest --test-dir C16_Desktop_Multimedia/build/u01 --output-on-failure
```

自动 smoke：

```powershell
C16_Desktop_Multimedia/build/u01/c16_quick.exe --self-check --fixtures C16_Desktop_Multimedia/build/fixtures
```

普通窗口可在 Local media path 输入本地路径并点击 Add，通过同一个 SessionController 导入媒体；失败提示显示在窗口底部。随后再选择列表项或使用播放控件。
