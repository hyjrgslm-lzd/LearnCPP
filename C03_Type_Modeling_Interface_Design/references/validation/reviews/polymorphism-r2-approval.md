# 作者B多态切片 r2 非作者复验批准

日期：2026-09-09。

## 结论

APPROVE。

作者B r1 审查中的三个代码/spec 阻断已关闭：

- L09 `Rectangle::clone()` 已补 `name()` 和 `dimensions()` 观察值检查。
- L09 新增 `validation_bad_wrong_value`，该坏例保持 clone 非空、地址独立、动态类型仍为 `Rectangle`，只把可观察尺寸改错；Debug/Release 都精确命中 `rectangle clone keeps observable dimensions`。
- L11 已加入 `alignas(64) OverAlignedShape`，checker 覆盖构造目标地址对齐、clone 地址对齐、dispatch、独立目标和销毁回收。
- L08 observation 与 L11 shared fixture 已显式 include `<utility>`，不再依赖 transitive include。

## 绑定版本

- Git HEAD: `8e0f407`
- 工作树状态：`C03_Type_Modeling_Interface_Design/` 为 untracked 课程目录，尚未提交。
- 作者B源码指纹入口：`C03_Type_Modeling_Interface_Design/references/validation/author-b-source-sha256.txt`
- 本轮关键源码指纹：
  - `908B4EE5294AC2A5F8EE0F1ED80167BA3722D9741ECD424472C22460918AD70D`  `exercises/L09_dynamic_polymorphism/checks/dynamic_shape_checks.cpp`
  - `33DB9C8709D721185460BB35A44EB5BBC1F1A79AC3B23DF6692D51EB7950E494`  `exercises/L09_dynamic_polymorphism/CMakeLists.txt`
  - `E12A108E6809C1B978C18600C744C1F4188DB52E4DD101AD26C6ADF4859CD03A`  `exercises/L09_dynamic_polymorphism/validation/bad_wrong_value/shape.hpp`
  - `B1C3071A66F254D8BF734FDA12DF2787BEC1CA427D74BCC9AC7A27386F6A944E`  `exercises/L11_type_erasure/checks/shape_types.hpp`
  - `F563F3C6FEAE813EAAC99148D1C83D21AA91E99685D911582D8EEF292F3EF394`  `exercises/L11_type_erasure/checks/any_shape_checks.cpp`
  - `1B8B6565F9F42AB192D37A2EBFD3978A3DA736115F239FFF2796EA4ACE3F0AE4`  `exercises/L08_composition/observation/composition_observation.cpp`
  - `9948E128C5E1876A41064B78D42C163106CD418721306AD0ED61C0E2B1E057C1`  `references/validation/author-b-report.md`

## 实际复验

只复验受影响 L08/L09/L11。L10 未变，未重跑。

通过项：

- `cmake --build build/c03-author-b-l08-debug --config Debug`：PASS
- `cmake --build build/c03-author-b-l08-release --config Release`：PASS
- `ctest --test-dir build/c03-author-b-l08-debug -C Debug --output-on-failure`：PASS，1/1
- `ctest --test-dir build/c03-author-b-l08-release -C Release --output-on-failure`：PASS，1/1
- `cmake --build build/c03-author-b-l09-debug --config Debug`：PASS，生成 `L09_dynamic_polymorphism_validation_bad_wrong_value.exe`
- `cmake --build build/c03-author-b-l09-release --config Release`：PASS，生成 `L09_dynamic_polymorphism_validation_bad_wrong_value.exe`
- `ctest --test-dir build/c03-author-b-l09-debug -C Debug --output-on-failure -V`：PASS，5/5；三个 negative 分别命中 `clone preserves dynamic type`、`clone owns an independent object`、`rectangle clone keeps observable dimensions`
- `ctest --test-dir build/c03-author-b-l09-release -C Release --output-on-failure -V`：PASS，5/5；三个 negative 同上
- `cmake --build build/c03-author-b-l11-debug --config Debug`：PASS
- `cmake --build build/c03-author-b-l11-release --config Release`：PASS
- `ctest --test-dir build/c03-author-b-l11-debug -C Debug --output-on-failure`：PASS，4/4
- `ctest --test-dir build/c03-author-b-l11-release -C Release --output-on-failure`：PASS，4/4

源码核对：

- `dynamic_shape_checks.cpp:49-50` 已检查 rectangle clone 的 `name()` 和 `dimensions()`。
- `bad_wrong_value/shape.hpp:42-44` 返回新的 `Rectangle(1, 1)`，因此不会被非空、地址独立、动态类型检查提前遮蔽。
- `shape_types.hpp:99-136` 定义 `alignas(64) OverAlignedShape`。
- `any_shape_checks.cpp:97-127` 检查目标地址与 clone 地址满足 `alignof(OverAlignedShape)`，并检查 dispatch 与 live count 回收。
- `composition_observation.cpp:5`、`shape_types.hpp:5` 已显式 include `<utility>`。

静态风险扫描：

- 命中项只在预期负例：`bad_alias` 的 `const_cast`、`bad_empty` 的空态 fallback。
- 未发现新增空 catch、硬编码 secret、吞错 fallback 或 best-effort 掩盖路径。

## 工具边界

本环境未暴露可调用 `lsp_diagnostics` / `ast_grep_search`。本批准不声称 LSP 已通过；以本任务获准规格中的 MSVC `/W4` Debug/Release 构建、CTest、源码核对与精确 negative 复验作为 next-best 诊断门。

## 剩余范围

本记录只批准作者B r2 修复闭环。整课公共导航、最终 Document 项目、ASan/frontier 全课门和跨模块最终审查仍由集成负责人处理。
