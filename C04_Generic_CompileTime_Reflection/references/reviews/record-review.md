# C04 P1 Static Record 非作者审查

审查时间：2026-09-09

受审 HEAD：`f261bea559d6722c31135fb2d3589be52fe958ed`

范围：
- `chapters/16-static-record.md`
- `exercises/P1_static_record/**`，排除 `build/**`
- 读取 `references/implementation-spec.md`、`CONTENT_REFACTORING_GUIDE.md`、`references/validation/p1-*.json` 作规格和作者证据

## 结论

ITERATE。

P1 的 C++23 手工 schema 核心路径、Reference/good/bad 检查器、known baseline、manual schema gap 观察、Student 初始失败语义，均有作者证据和本轮独立证据支撑。但 frontier 能力路径在本机开启 `GENERIC_STUDY_ENABLE_FRONTIER=ON` 后构建失败，未能进入规格要求的“缺少真实反射能力时返回 77 并由 CTest 标记 SKIP”状态。

## 阻断问题

### [HIGH] frontier 缺能力分支自身编译失败，不能按规格 SKIP

文件：`C04_Generic_CompileTime_Reflection/exercises/P1_static_record/checks/reflection_driver.cpp:12`

触发：

```powershell
cmake -S C04_Generic_CompileTime_Reflection/exercises/P1_static_record -B C04_Generic_CompileTime_Reflection/exercises/P1_static_record/build/record-review-frontier -G "Visual Studio 18 2026" -A x64 -DGENERIC_STUDY_ENABLE_FRONTIER=ON
cmake --build C04_Generic_CompileTime_Reflection/exercises/P1_static_record/build/record-review-frontier --config Debug
```

实际结果：构建 `P1_static_record_reflection` 失败，证据见 `references/validation/record-review/08-frontier-build-debug.json`。MSVC 报：

```text
reflection_driver.cpp(12,61): error C3861: "__has_include": 找不到标识符
reflection_driver.cpp(12,76): error C2065: "meta": 未声明的标识符
```

根因：`__has_include(<meta>)` 只能用于预处理条件，当前代码在 `#else` 的运行时输出表达式里再次使用它：

```cpp
std::cout << "SKIP P1 real reflection: meta header=" << __has_include(<meta>);
```

影响：规格和 README 都要求本机缺少 meta/反射/展开能力时，该分支返回 77，且 CTest 用 `SKIP_RETURN_CODE 77` 区分能力缺失；当前结果是 build FAIL，导致 frontier 预设不能验证，也不能作为“真实反射主体未编译/运行”的干净证据。

最小修复：在预处理阶段把探测结果落成普通宏或常量，再在 `main()` 中打印该值。例如：

```cpp
#if __has_include(<meta>)
#define C04_RECORD_HAS_META_HEADER 1
#else
#define C04_RECORD_HAS_META_HEADER 0
#endif
```

随后用 `C04_RECORD_HAS_META_HEADER` 参与能力条件和输出。修复后重跑 frontier configure/build/CTest，期望本机返回 77 并显示 `real backend not compiled or run`；若某编译器声明全部能力后编译 `src/reflection/record_ops.hpp` 失败，应保留为 FAIL。

## 已验证通过项

- `references/validation/record-review/01-configure.json`：P1 独立 MSVC Debug 配置 PASS。
- `references/validation/record-review/02-build-debug.json`：P1 独立 MSVC Debug 构建 PASS。
- `references/validation/record-review/03-ctest-debug.json`：CTest 5/5 PASS，覆盖 Reference、validation_good、validation_bad_rejected、known_baseline、schema_gap。
- `references/validation/record-review/04-student-configure.json`：Student-only 配置 PASS，`GENERIC_STUDY_BUILD_REFERENCE=OFF`、`GENERIC_STUDY_TEST_STUDENTS=ON`。
- `references/validation/record-review/05-student-build-debug.json`：Student-only 构建 PASS。
- `references/validation/record-review/06-student-ctest-debug-expected-fail.json`：Student 初始实现以 exit 8 失败，并包含 `check failed: encoded fields cover the complete schema`，符合 Starter 未完成语义。
- `references/validation/record-review/07-frontier-configure.json`：frontier 配置 PASS。

## 静态审查摘要

教学正文与 README 沿“已知类型基线 -> member pointer/schema -> visit_fields -> codec -> format -> 真实反射边界”递进，能遮住 Reference 后给出实现线索。manual schema gap 明确是违反完整性契约的安全观察，没有误判为 Reference bug。

`src/reference/record_ops.hpp` 使用手工 schema tuple 生成字段和名称；`src/reference/codec.hpp` 提供拥有型 `FieldValue`、`expected<T, field_error>` 校验错误、局部构造、`to_chars/from_chars`、bool 严格 `true/false`、string 原样、empty schema 与未知字段处理。`checks/record_checks.hpp` 覆盖字段身份、声明顺序、cv/ref、右值移动、callback 左值重复调用、noexcept 与真实表达式一致、异常停止后续字段且不回滚已发生副作用。

Student/Reference/good/bad 接线独立：`c04_add_exercise` 为各变体设置不同 include 目录，`record_checks.cpp` 只包含当前 include 路径下的 `record_ops.hpp`。`validation/good` 使用独立 index sequence 遍历；`validation/bad` 只在 codec 中故意丢最后字段，负测匹配精确 checker 文本。

`src/reflection/record_ops.hpp` 静态上使用 `std::meta::nonstatic_data_members_of`、`annotations_of_with_type`、splicing `object.[:member:]`、`template for`，并只用 schema 限定教学域，没有用手工 tuple 冒充反射。该主体本机未编译，不能声明真实反射源码通过。

一手语法核对来源：
- WG21 P2996R13 Reflection for C++26: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html
- WG21 P3394R4 Annotations for Reflection: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3394r4.html
- WG21 P3547R0 Modeling Access Control With Reflection: https://open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3547r0.html

## 未阻断限制

- 本环境没有可用 LSP diagnostics 工具；本轮用 MSVC/CMake/CTest 作为实际编译诊断。由于 verdict 是 ITERATE，未因缺 LSP 作 APPROVE。
- 未运行 ASan；P1 当前审查阻断在 frontier build 前置失败，ASan 不会关闭该问题。

## Recommendation

ITERATE。修复 `reflection_driver.cpp:12` 的能力缺失输出后，至少重跑 frontier configure/build/CTest，并保留新的非作者证据。核心 C++23 路径目前未发现阻断。
