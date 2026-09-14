# 08 可访问性：名称、角色、动作与键盘路径

可访问性不是最后给控件补几句 tooltip。屏幕阅读器、键盘用户和自动化测试依赖的是对象名称、角色、状态、值和动作。MediaWorkbench 的本地审阅器至少要让播放按钮、时间轴、媒体列表这些核心控件可被识别和操作。

Qt Widgets 的基础控件已有 accessibility bridge。`QPushButton` 通常报告 Button role，`QSlider` 报告 Slider role。但默认文本不一定足够：一个只显示 `>` 的播放按钮，对视觉用户可能清楚，对辅助技术只是“>”。应设置实际 `accessibleName` 和必要的 `accessibleDescription`：

```cpp
play->setAccessibleName("播放或暂停预览");
play->setAccessibleDescription("切换 MediaWorkbench 本地预览的播放状态");
slider->setAccessibleName("媒体时间轴");
slider->setFocusPolicy(Qt::StrongFocus);
label->setBuddy(slider);
```

键盘路径和语义同等重要。控件没有 `TabFocus`，键盘用户到不了；label 没有 buddy，助记键无法把焦点移动到目标控件；按钮没有描述，读屏用户可能知道“有一个 Button”，却不知道它是播放还是提交。检查既看 Qt focus policy，也看 `QAccessibleInterface::state().focusable`，因为视觉 Tab 顺序和可访问树状态要一致。

自定义控件是更容易漏的地方。假设 MediaWorkbench 后续画一个波形时间轴：如果它只是 `QWidget::paintEvent` 里的彩色矩形，辅助技术只看到一个普通 Client 区域。正确做法有两条：优先组合 `QSlider`、`QLabel`、`QPushButton` 这类已有语义控件；确实需要自绘时，实现对应的 accessible interface，至少暴露 Name、Role、Value、可执行 Action、当前状态和键盘操作。自定义控件的语义不是 tooltip，tooltip 给看得见鼠标的人；accessible name/description/value 给辅助技术和键盘路径。

自动检查能验证 Qt 层 accessibility object：

```cpp
auto iface = QAccessible::queryAccessibleInterface(button);
iface->role() == QAccessible::Button;
iface->text(QAccessible::Name) == "播放或暂停预览";
iface->text(QAccessible::Description).contains("预览");
```

这个检查不等于 Windows UIA、macOS VoiceOver 或真实屏幕阅读器验收。它证明 Qt 对象暴露了基本语义：role 是 Button/Slider，name 可读，description 解释动作，state 标记可聚焦，slider value 能被读取。系统桥接、读屏朗读顺序、平台主题差异仍需人工或平台工具验证。本项目不把 offscreen 自动检查冒充完整无障碍认证。

可访问性还会反向影响模型和状态。媒体列表 item 的 display text 要能表达身份，选择变化要能被视图和辅助技术看到；时间轴的 value text 要反映当前位置；播放/暂停状态改变后名称或状态也要更新。P1 中如果做自定义波形或缩略图控件，必须给出同等语义出口。

练习 [L08_accessibility](../exercises/L08_accessibility/README.md) 创建一个小型审阅面板。检查器在 offscreen 下验证按钮和 slider 的 accessible name、description、role、focus policy、accessible focusable state、label buddy 和 value text。bad 版本只有视觉符号和无焦点 slider，会被拒绝。它依赖 L02 的 widget parent tree、L06 的视图身份，以及 L07 的 DPI/i18n 状态恢复。
