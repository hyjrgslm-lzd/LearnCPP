# 作者B报告：08-11 组合、多态与类型擦除

日期：2026-09-09。范围限 `chapters/08-*` 到 `11-*`、`exercises/L08_composition`、`L09_dynamic_polymorphism`、`L10_static_polymorphism`、`L11_type_erasure`，以及本目录 `author-b-*` 证据。未修改公共 CMake、helper、根 README、总计划或其他作者文件。

## 内容完成

- 08 章完成组合/继承、LSP、`public`/`private` 继承、NVI、构造期虚调用和析构责任讲解；L08 为观察型程序。
- 09 章完成 dynamic polymorphism、`virtual`/`override`/`final`、虚析构、切片、RTTI、clone、多继承/虚基语义边界；L09 为实现题。
- 10 章完成函数模板、concept/requires、语义公理、重载/实例化、CRTP、dependent-base `this->` 和构造/析构期风险；L10 为观察型程序。
- 11 章完成 heap-only owning `AnyShape`、操作表 clone/destroy/dispatch、空态、copy/move/self-assignment、const dispatch、失败保证与 MSVC `<functional>` 源码导读边界；L11 为实现题。

## 实验边界

- L09 checker 覆盖非法尺寸、Circle 直径溢出拒绝、基类虚派发、Rectangle/Circle clone 非空、clone 独立、动态类型保持、可观察值保持、虚析构释放派生对象。
- L09 默认坏例只破坏 Circle clone 的动态类型；额外 `validation_bad_alias` 只破坏 Rectangle clone 的独立拥有语义；额外 `validation_bad_wrong_value` 只破坏 Rectangle clone 的可观察值。
- L11 checker 覆盖空态 checked throw、const dispatch、深复制、移动后空态、自赋值、copy-and-swap 失败保持旧目标、析构计数、`alignas(64)` 目标构造/复制后的实际地址对齐、dispatch 和销毁。
- L11 默认坏例只破坏 copy assignment 的 strong guarantee；额外 `validation_bad_empty` 只破坏空态 checked throw。
- `AnyShape` 固定 heap-only；未实现 SBO、allocator 参数、运行时插件注册或性能排名。

## Fresh MSVC 证据

Debug 叶级：

- `author-b-l08-configure-debug.json` / `author-b-l08-build-debug.json` / `author-b-l08-ctest-debug.json`：PASS，L08 observation 1/1。
- `author-b-l09-configure-debug.json` / `author-b-l09-build-debug-r3.json` / `author-b-l09-ctest-debug-r4.json`：PASS，L09 reference、good、两个 negative 共 4/4。
- `author-b-l10-configure-debug.json` / `author-b-l10-build-debug.json` / `author-b-l10-ctest-debug.json`：PASS，L10 observation 1/1。
- `author-b-l11-configure-debug.json` / `author-b-l11-build-debug-r2.json` / `author-b-l11-ctest-debug-r2.json`：PASS，L11 reference、good、两个 negative 共 4/4。

Release 叶级：

- `author-b-l08-configure-release.json` / `author-b-l08-build-release.json` / `author-b-l08-ctest-release.json`：PASS，L08 observation 1/1。
- `author-b-l09-configure-release.json` / `author-b-l09-build-release-r2.json` / `author-b-l09-ctest-release-r2.json`：PASS，L09 4/4。
- `author-b-l10-configure-release.json` / `author-b-l10-build-release.json` / `author-b-l10-ctest-release.json`：PASS，L10 observation 1/1。
- `author-b-l11-configure-release.json` / `author-b-l11-build-release.json` / `author-b-l11-ctest-release.json`：PASS，L11 4/4。

Student ref-off：

- `author-b-l09-configure-student.json` / `author-b-l09-build-student-r2.json`：PASS；`author-b-l09-ctest-student-r2.json` 按预期失败，诊断为 `check failed: rectangle clone returns an owning pointer`。
- `author-b-l11-configure-student.json` / `author-b-l11-build-student.json`：PASS；`author-b-l11-ctest-student.json` 按预期失败，诊断为 `check failed: copy failure keeps target unchanged`。

## Review fix 复验

c03_polymorphism_review 的三项 ITERATE 已在作者B范围内修复：

- L09 Rectangle clone 增加 name/dimensions 一致性检查，并新增 `validation_bad_wrong_value`。`author-b-reviewfix-l09-ctest-debug.json` 与 `author-b-reviewfix-l09-ctest-release.json` 均 PASS，5/5，三个 negative 全部被精确拒绝。
- L11 `shape_types.hpp` 增加 `alignas(64) OverAlignedShape`，checker 验证构造与复制后的 `target_address()` 实际满足 `alignof(OverAlignedShape)`，并覆盖 name/dimensions dispatch 和销毁。`author-b-reviewfix-l11-ctest-debug.json` 与 `author-b-reviewfix-l11-ctest-release.json` 均 PASS，4/4。
- L08 observation 与 L11 shared fixture 补显式 `<utility>`。`author-b-reviewfix-l08-build-debug.json` 与 `author-b-reviewfix-l08-build-release.json` 均 PASS。

## 指纹

- 作者B源码指纹：`author-b-source-sha256.txt`。
- MSVC `<functional>` 本机源码导读输入：`author-b-msvc-functional-source.md`。

## 剩余边界

本报告只冻结作者B切片。整课公共导航、标准/实现矩阵、最终 Document 项目、ASan/frontier 全课门和非作者独立审查仍由集成负责人处理。
