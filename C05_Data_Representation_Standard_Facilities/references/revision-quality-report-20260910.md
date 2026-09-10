# C05 2026-09-10 fmt/spdlog增量质量报告

本报告只记录第19/20章与U02/U03增量。旧[quality-report.md](quality-report.md)中的33/34项矩阵是C05历史基线；fmt/spdlog新增范围以本文和root后续最终矩阵为准。

## 范围与知识点

| 范围 | 教学点 | 证据入口 |
|---|---|---|
| 第19章 / U02 fmt | fmt 12.1.0固定源码；`fmt::format_string`和`fstring`在实例化点做编译期校验；`fmt::runtime`显式进入运行期错误；自定义formatter只是展示策略；`FMT_COMPILE`和`_cf`把字面量解析成编译期表示；`make_format_args`和`dynamic_format_arg_store`区分借用与拥有 | [revision-format-author.md](reviews/revision-format-author.md)、[revision-format-review.md](reviews/revision-format-review.md)、[format-author-source-hashes-reviewfix.json](validation/revision-20260910/format-author-source-hashes-reviewfix.json) |
| 第20章 / U03 spdlog | spdlog 1.17.0同步logger前端；局部logger和sink；`SPDLOG_ACTIVE_LEVEL`预处理裁剪与运行时level过滤分开；fmt/std两种后端的formatter和错误类型；pattern compile是运行期配置处理；logger `error_handler`与后端format error分开 | [revision-format-author.md](reviews/revision-format-author.md)、[revision-format-review.md](reviews/revision-format-review.md) |
| 依赖与后端 | fmt commit `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f`；spdlog commit `79524ddd08a4ec981b7fea76afd08ee05f83755d`；默认扩展OFF，显式ON缺失/错版/target污染应FAIL；fmt backend使用外部静态fmt，std backend独立配置 | [dependencies-format summary](validation/revision-20260910/dependencies-format-20260910-123030/summary.json)、[C04依赖r2复验](../../C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependency-review-r2-summary.json) |

本增量只讲格式化和同步日志前端。async queue、overflow策略、flush/shutdown生命周期和服务观测登记给C08/C11后续；本文不把这些系统行为写成已覆盖。

## 审查与修复闭环

作者记录显示U02/U03初始实现已覆盖Release/Debug、fmt/std backend、编译期负例和可观测输出，但保留了若干非最终证据：U02早期wrapper形状错误、诊断setup未传`DATA_STUDY_ENABLE_FORMAT_LIBS=ON`、诊断pattern过窄、U03 throwing formatter初稿返回类型错误。这些记录保留为过程证据，不作为最终通过依据。

非作者`revision-format-review.md`初审ITERATE，两个MEDIUM问题已关闭：

- U02只验证dynamic store copy，未验证显式`std::ref`/`std::cref`借用。R2 closeout记录当前观察程序同时输出`owned_text=owned=9`和`borrowed_text=Borrowed`，说明普通值被复制，显式借用能观察源对象后续修改。
- U03用`catch (const std::exception&)`判断runtime format error，类型过宽。R2 closeout记录fmt backend只捕获`fmt::format_error`，std backend只捕获`std::format_error`，自定义formatter抛`std::runtime_error`仍只由logger error_handler观察。

R2 closeout结论为APPROVE，验证记录位于`references/validation/revision-20260910/format-blind/closeout-author-*`。其中U02 Debug/Release各2/2 PASS；U03 fmt/std backend Debug/Release各3/3 PASS，并要求出现`backend_runtime_error=1`、`logger_error_handler_calls=1`、`active_macro_calls=0`和`active_macro_calls=1`。

## 当前可引用证据

- `format-u02-ctest-release-final.json`：U02 Release 2/2 PASS，输出含`fixed_text=id=7`、`runtime_error=1`、`compiled_text=port:443`、`cf_text=answer=42`、`erased_text=items=3`。
- `format-blind/closeout-author-u02-ctest-release-r2.json`：U02 R2 Release PASS，额外要求`owned_text=owned=9`和`borrowed_text=Borrowed`。
- `format-u03-fmt-ctest-release-final.json`：U03 fmt backend Release 3/3 PASS，含DEBUG/INFO两个active level目标和编译期负例。
- `format-blind/closeout-author-u03-std-ctest-release-r2.json`：U03 std backend Release PASS，输出含`backend=std`、`backend_runtime_error=1`、`logger_error_handler_calls=1`、DEBUG宏求值差异。

## 最终根构建矩阵

| 配置 | 实际结果 | 证据 |
|---|---|---|
| 扩展OFF，核心Release | 33 PASS，0 FAIL | [配置](validation/revision-20260910/root-core-configure-01.json)、[构建](validation/revision-20260910/root-core-build-01.json)、[CTest](validation/revision-20260910/root-core-ctest-01.json) |
| external fmt backend，Release | 38 PASS，0 FAIL | [构建](validation/revision-20260910/root-fmt-build-01.json)、[CTest](validation/revision-20260910/root-fmt-ctest-01.json) |
| external fmt backend，Debug | 38 PASS，0 FAIL | [构建](validation/revision-20260910/root-fmt-debug-build-01.json)、[CTest](validation/revision-20260910/root-fmt-debug-ctest-01.json) |
| std backend，Release | 38 PASS，0 FAIL | [构建](validation/revision-20260910/root-std-build-01.json)、[CTest](validation/revision-20260910/root-std-ctest-01.json) |
| std backend，Debug | 38 PASS，0 FAIL | [构建](validation/revision-20260910/root-std-debug-build-01.json)、[CTest](validation/revision-20260910/root-std-debug-ctest-01.json) |

这些计数覆盖原核心与本次两个观察单元；观察通过不等于学习者已完成迁移题。独立迁移及r2复验见上文，依赖污染负控已由[C04/C05依赖r2审查](../../C04_Generic_CompileTime_Reflection/references/reviews/revision-dependencies-review-r2.md)关闭。本轮未重跑无关ICU/前沿专项，原结果继续保留为历史基线。完整交付文件与指纹见[C04/C05联合清单](../../C04_Generic_CompileTime_Reflection/references/revision-delivery-manifest.md)。
