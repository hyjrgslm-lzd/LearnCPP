# L08 accessibility

对应正文：[08-accessibility](../../chapters/08-accessibility.md)。

学生只编辑 `student/solution.hpp`。实现 `createReviewPanel`：

1. 创建本地审阅器的播放按钮、时间轴 label、时间轴 slider。
2. 设置对象名，供检查器定位控件。
3. 设置实际 `accessibleName`、`accessibleDescription`、focus policy。
4. label 使用 buddy 指向 slider。
5. offscreen 下用 `QAccessible::queryAccessibleInterface` 检查角色和文本。

本题只自动检查 Qt accessibility object，不冒充 Windows UIA 或真实屏幕阅读器验收。
