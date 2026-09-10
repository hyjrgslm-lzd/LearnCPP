# C04 revision frontier author note

本次只实施已批准前沿修正，未修改chapter10、root CMake、依赖脚本、commit或push。

改动：

- `chapters/14-splicing-generation.md`：补充`template for`的`auto`与`constexpr auto`差异、类型域经反射值域展开、`break`/`continue`只控制执行流而不豁免后续展开实例的语义约束。
- `chapters/15-annotations-frontier.md`：补充P2747R2 constexpr placement new和P3822R2 conditional `noexcept`复合要求，明确C++26/C++29/未采纳提案分层。
- `exercises/F01_frontier/CMakeLists.txt`：新增P2747R2和P3822R2最小行为探测，探测通过才给主体目标定义能力宏。
- `exercises/F01_frontier/probes/constexpr_placement_new_probe.cpp`：新增P2747R2主体正例，保留`C04_P2747_NEGATIVE_WRONG_STORAGE`负例入口。
- `exercises/F01_frontier/probes/c29_conditional_noexcept_requirement_probe.cpp`：新增P3822R2主体正例和四格语义检查，保留`C04_P3822_NEGATIVE_NON_BOOL_CONDITION`负例入口。
- `exercises/F01_frontier/README.md`、`answers.md`、`references/standards-and-implementations.md`：同步新增probe判读和规范边界。

验证：

- `references/validation/revision-20260910/frontier-configure-f01-p2747-p3822-02.json`：PASS。
- `references/validation/revision-20260910/frontier-build-f01-p2747-p3822-02.json`：PASS。
- `references/validation/revision-20260910/frontier-ctest-f01-p2747-p3822-02.json`：PASS，14/14 F01 tests registered，0 failed，14 skipped。
- `references/validation/revision-20260910/frontier-build-f01-force-placement-02.json`：PASS证据，期望exit 1，强制P2747主体失败。
- `references/validation/revision-20260910/frontier-build-f01-force-noexcept-02.json`：PASS证据，期望exit 1，强制P3822主体失败。

保留边界：当前MSVC 19.51未通过P2747/P3822正例探测，所以本机不能声称两个主体或标准负例已经通过；只能声称源码、门控、SKIP/FAIL分类和force控制路径已验证。
