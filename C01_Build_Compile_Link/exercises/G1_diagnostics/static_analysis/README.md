# static_analysis

这里放一个只用于静态分析观察的最小文件。它不进入默认 CTest，因为本章要区分三类信号：

- 编译器警告和 static analyzer：不运行程序，只检查路径敏感的潜在问题。
- ASan：运行程序，在真实越界发生时报告。
- libFuzzer：生成输入并重复运行入口函数，用有限 `-runs` 和 timeout 保证课程验证可控。


在整章 `vs-study` 解决方案中，可从 `G1_diagnostics_student → Experiments → static_analysis` 打开 `null_state.cpp`。它仅供浏览，运行学生项目不会执行静态分析；工具选择和验证步骤见[本题说明](../README.md)。
