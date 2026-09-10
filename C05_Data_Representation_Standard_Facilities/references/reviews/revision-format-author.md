# C05 fmt/spdlog author handoff

作者范围：`chapters/19-fmt-library.md`、`chapters/20-spdlog-frontend.md`、`exercises/U02_fmt/**`、`exercises/U03_spdlog/**`，以及 `chapters/09-formatting-and-locales.md`、`chapters/17-source-and-downstream.md` 的桥接段。未修改 C05 根 README、顶层 `exercises/CMakeLists.txt`、presets、coverage 或公共 `FormatLibraries.cmake`。

## 内容

- `19-fmt-library.md`：固定 fmt 12.1.0 commit `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f`，讲 `fstring`/`format_string` 的非运行时普通字符串路径、consteval checker、参数类型映射、`fmt::runtime`、自定义 formatter、`FMT_COMPILE`/`_cf` 生成 `text`/`field`/`concat` 的手推链，以及 `make_format_args`/`dynamic_format_arg_store` 的拥有与借用边界。
- `20-spdlog-frontend.md`：固定 spdlog v1.17.0 commit `79524ddd08a4ec981b7fea76afd08ee05f83755d`，限定同步 logger 前端，讲局部 logger+sinks、sink 持有 formatter、`SPDLOG_ACTIVE_LEVEL` 宏裁剪、`logger::log_` 运行时过滤、后端 formatter 归属、pattern runtime 配置和 `error_handler` 出口。
- `U02_fmt`：观察程序覆盖固定格式串返回文本、runtime 错误、自定义 formatter、`FMT_COMPILE`、`_cf`、type-erased args、动态参数存储；编译负例用 C04 `compile_case.py --setup`，control 先过，再要求 subject 出现 MSVC/fmt12 的 immediate-function 诊断。
- `U03_spdlog`：同一源码构建 DEBUG/INFO 两个 active-level 目标，验证宏裁剪 0 次求值、运行时过滤仍求值但不写 sink；fmt/std backend 都验证自定义 formatter、后端 runtime error、logger error handler、pattern 输出和 commit/backend 定义。

## 验证

所有命令在 `C05_Data_Representation_Standard_Facilities/exercises` 下用 C02 `record_process.py` 记录，证据位于 `references/validation/revision-20260910/`。

| 范围 | 记录 | 结果 |
| --- | --- | --- |
| U02 fmt Release | `format-u02-build-release-observable.json`, `format-u02-ctest-release-observable.json` | PASS；2/2 tests passed |
| U02 fmt Debug | `format-u02-build-debug-observable.json`, `format-u02-ctest-debug-observable.json` | PASS；2/2 tests passed |
| U03 spdlog fmt Release | `format-u03-fmt-build-release-observable.json`, `format-u03-fmt-ctest-release-observable.json` | PASS；3/3 tests passed |
| U03 spdlog fmt Debug | `format-u03-fmt-build-debug-observable.json`, `format-u03-fmt-ctest-debug-observable.json` | PASS；3/3 tests passed |
| U03 spdlog std Release | `format-u03-std-build-release-observable.json`, `format-u03-std-ctest-release-observable.json` | PASS；3/3 tests passed |
| U03 spdlog std Debug | `format-u03-std-build-debug-observable.json`, `format-u03-std-ctest-debug-observable.json` | PASS；3/3 tests passed |
| C05 core OFF configure after root wiring | `format-core-off-configure-after-u02-u03.json` | PASS；`U02_fmt`/`U03_spdlog` report disabled when `DATA_STUDY_ENABLE_FORMAT_LIBS=OFF` |

Representative observable output from the final Release records:

- U02: `fixed_text=id=7`, `runtime_error=1`, `metric_text=cpu=85.375`, `compiled_text=port:443`, `cf_text=answer=42`, `erased_text=items=3`, `owned_text=owned=9`.
- U03 fmt backend: DEBUG target `active_macro_calls=1`; INFO target `active_macro_calls=0`; both show `member_debug_calls=1`, `backend_runtime_error=1`, `logger_error_handler_calls=1`, `sink_text=[info] cpu=85%`.
- U03 std backend: same call-count and error-channel observations with `backend=std`.

Known author-side non-final evidence kept append-only:

- `format-u02-build-release.json`: initial wrapper used a non-forwarding `fmt::format_string<std::string_view, int>` shape that fmt12/MSVC rejected.
- `format-u02-ctest-release.json`: diagnostic setup initially did not pass `DATA_STUDY_ENABLE_FORMAT_LIBS=ON` into the inner CMake.
- `format-u02-ctest-release-r2.json`: diagnostic pattern was too narrow for MSVC/fmt12 `C7595` immediate-function output.
- `format-u03-fmt-build-release.json`: throwing formatter initially lacked an output iterator return type required by fmt.

## Boundaries

No async spdlog queue, overflow policy, flush/shutdown lifecycle, throughput benchmark, machine install, commit, push, credentials, or production system touched. New units are observation/migration teaching units, not Student/Reference implementation exercises.

## Review fixes

Non-author review `revision-format-review.md` returned ITERATE with two medium findings. Author fixes:

- U02 now verifies both `dynamic_format_arg_store` copy and explicit `std::cref` borrowing: copied source mutation leaves `owned_text=owned=9`; borrowed source mutation is observed as `borrowed_text=Borrowed` while the source object is still alive.
- U03 now catches backend runtime format errors by exact backend type: `fmt::format_error` for external fmt and `std::format_error` for std backend. The custom formatter `std::runtime_error` path remains separate and is observed through `logger_error_handler_calls=1`.
