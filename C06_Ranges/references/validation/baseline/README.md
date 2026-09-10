# 重构前基线

2026-09-10：当前MSVC 19.51.36256.0 / STL 145、202604 / CMake 4.2.3，在既有C++26配置下构建全部29个入口成功；`ctest -N`列出0项。G1、H3、CAPSTONE4仅输出骨架提示。这是编译入口证据，不是课程行为验收。

`msvc-release-build.txt`为规划阶段真实原始构建输出，源树在本批开工前尚未变更。旧构建程序位于本机忽略的`exercises/build-codex-review-20260910`，不会随仓库交付。

`h3-before-source.txt`是H3修改前的完整冻结源文件，**含已知错误，不作为正常示例编译或运行**。`h3-const-before.cpp`只做编译负例：随机访问concept断言失败，并对按值transform结果触发返回临时对象引用警告。它不运行真实未定义行为。

复现命令与原始结果见`h3-const-gcc-before.json`。记录器的PASS表示成功得到预期的编译拒绝和诊断，不表示旧实现正确。generator默认移动复制句柄的错误由唯一所有权契约与析构路径共同判定，未声称已实测double-free崩溃；后续用受控销毁计数与ASan验证修复。
