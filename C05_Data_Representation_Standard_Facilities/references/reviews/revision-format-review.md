# C05 fmt/spdlog blind reviewer report

Verdict: ITERATE

本轮教材主线、上游源码定位、U02/U03 作者复验都成立；但 U02 对 `dynamic_format_arg_store` 的显式 `std::ref`/`std::cref` 借用路径没有可运行证据，U03 的 runtime format error 捕获过宽。两者都不影响现有目标编译通过，但会削弱本增量要求的“参数存储边界”和“错误通道”教学精度。

## Files reviewed

- `chapters/09-formatting-and-locales.md`
- `chapters/19-fmt-library.md`
- `chapters/20-spdlog-frontend.md`
- `exercises/U02_fmt/README.md`
- `exercises/U02_fmt/observation.cpp`
- `exercises/U02_fmt/CMakeLists.txt`
- `exercises/U02_fmt/diagnostics/good_compile_format.cpp`
- `exercises/U02_fmt/diagnostics/bad_compile_format.cpp`
- `exercises/U03_spdlog/README.md`
- `exercises/U03_spdlog/observation.cpp`
- `exercises/U03_spdlog/CMakeLists.txt`
- `exercises/U03_spdlog/diagnostics/good_compile_format.cpp`
- `exercises/U03_spdlog/diagnostics/bad_compile_format.cpp`
- `exercises/cmake/FormatLibraries.cmake`
- `references/reviews/revision-format-author.md`
- fmt 12.1.0 upstream headers: `include/fmt/base.h`, `include/fmt/args.h`, `include/fmt/compile.h`
- spdlog 1.17.0 upstream headers: `include/spdlog/common.h`, `include/spdlog/logger.h`, `include/spdlog/logger-inl.h`, `include/spdlog/pattern_formatter-inl.h`, `include/spdlog/sinks/base_sink-inl.h`

## Severity summary

- CRITICAL: 0
- HIGH: 0
- MEDIUM: 2
- LOW: 0

## Issues

### [MEDIUM] U02 只验证 dynamic store copy，没有验证显式 ref 借用

File: `C05_Data_Representation_Standard_Facilities/exercises/U02_fmt/observation.cpp:76`

Issue: README 和正文都把 `dynamic_format_arg_store` 的 copy/ref 边界列为学习目标：`README.md:12` 写明显式传 `std::ref` 时仍是借用，`README.md:14` 要求说明哪些参数复制、哪些通过 `std::ref` 借用；正文 `chapters/19-fmt-library.md:82` 也强调显式 `std::ref(x)` 是借用。但观察程序当前只做：

```cpp
fmt::dynamic_format_arg_store<fmt::format_context> owned;
owned.push_back(std::string("owned"));
owned.push_back(9);
```

这能证明动态 store 可以拥有复制进去的值，不能证明显式 `std::ref`/`std::cref` 借用路径，也不能证明借用对象的生命周期要求。

Risk: 学员看到运行输出 `owned_text=owned=9` 会以为动态 store 边界已经被完整验证，但最危险的长期保存参数列表场景正是显式借用。这个缺口和本次审查要求里的 `dynamic store显式ref` 直接冲突。

Fix: 在 U02 observation 增加一个最小 `std::cref` 案例。建议用上游 fmt 文档同型的 `char[]` 或一个稳定 custom object，避免 `std::string` 重新赋值导致 buffer 语义噪声。例如：

```cpp
char band[] = "Rolling Stones";
fmt::dynamic_format_arg_store<fmt::format_context> mixed;
mixed.push_back(std::string("copied"));
mixed.push_back(std::cref(band));
band[9] = 'c';
check(fmt::vformat("{}|{}", mixed) == "copied|Rolling Scones",
    "dynamic_format_arg_store copies normal values and borrows explicit refs");
```

### [MEDIUM] U03 runtime format error 捕获过宽，不能证明错误类型

File: `C05_Data_Representation_Standard_Facilities/exercises/U03_spdlog/observation.cpp:77`

Issue: `backend_rejects_runtime_format()` 用 `catch (const std::exception&)` 判断后端 runtime format error。正文 `chapters/20-spdlog-frontend.md:53` 区分 `fmt::format_error`、`std::format_error` 和 formatter 自己抛出的异常；当前 broad catch 会让任意 `std::exception` 都通过检查。

Risk: 这是教学观察程序，不是生产错误恢复；过宽 catch 会抹掉“格式后端错误通道”的类型证据。若将来 std/fmt 后端配置、输入构造或 formatter 代码导致其他异常，这个 check 仍可能给出 `backend_runtime_error=1` 的假阳性。

Fix: 按 backend 精确捕获：

```cpp
#ifdef SPDLOG_USE_STD_FORMAT
    } catch (const std::format_error&) {
#else
    } catch (const fmt::format_error&) {
#endif
        return true;
    }
```

formatter 自己抛出的 `std::runtime_error` 已由 `Exploding` + `logger.set_error_handler` 单独覆盖，保留在另一条检查里即可。

## Evidence

Blind implementation and SHA evidence saved under `references/validation/revision-20260910/format-blind/`.

- Blind configure/build/run:
  - `configure-r2.json`: PASS
  - `build-good-release-r3.json`: PASS
  - `build-good-debug-r2.json`: PASS
  - `run-fmt-release-r3.json`, `run-fmt-debug-r2.json`: PASS
  - `run-spdlog-*-release-r3.json`, `run-spdlog-*-debug-r2.json`: PASS for fmt/std and DEBUG/INFO
  - `build-bad-fmt-release-r2.json`, `build-bad-spdlog-release-r2.json`: PASS as expected failure with `C7595`
- Author reverify:
  - `reverify-author-u02-release.json`: PASS, 2/2
  - `reverify-author-u02-debug.json`: PASS, 2/2
  - `reverify-author-u03-fmt-release.json`: PASS, 3/3
  - `reverify-author-u03-fmt-debug.json`: PASS, 3/3
  - `reverify-author-u03-std-release.json`: PASS, 3/3
  - `reverify-author-u03-std-debug.json`: PASS, 3/3
- Static search:
  - no hardcoded secret pattern found in U02/U03 slice
  - no global/default spdlog logger use found in U03 slice
  - `format_args`, `dynamic_format_arg_store`, `std::ref/std::cref`, broad catch, and fallback-like patterns searched with `rg`

Tool limitation: no callable `lsp_diagnostics` or `ast_grep_search` tool was available after tool search. I used MSVC CMake/CTest reverify plus targeted `rg` static checks instead. This lane should not be treated as a full LSP-backed approval gate.

## Recommendation

ITERATE. Fix the two medium teaching-precision gaps, then rerun U02 Debug/Release and U03 fmt/std Debug/Release. No CRITICAL or HIGH code/security issue found in this slice.

## R2 closeout

Verdict: APPROVE for this fmt/spdlog slice.

The two R1 issues are closed:

- `exercises/U02_fmt/observation.cpp:77-90` now checks dynamic store copy/ref behavior separately. `copied[0] = 'X'` leaves `owned_text=owned=9`, while `std::cref(borrowed)` observes the later mutation as `borrowed_text=Borrowed`.
- `exercises/U03_spdlog/observation.cpp:69-82` now catches `std::format_error` only for the std backend and `fmt::format_error` only for the fmt backend. The `Exploding` formatter's `std::runtime_error` remains isolated to the logger `error_handler` check at `observation.cpp:119-126`.

R2 validation evidence, all recorded with `C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py` under `references/validation/revision-20260910/format-blind/`:

- U02 current source:
  - `closeout-author-u02-build-release-r2.json`: PASS
  - `closeout-author-u02-ctest-release-r2.json`: PASS, required `owned_text=owned=9` and `borrowed_text=Borrowed`
  - `closeout-author-u02-build-debug-r2.json`: PASS
  - `closeout-author-u02-ctest-debug-r2.json`: PASS, required `owned_text=owned=9` and `borrowed_text=Borrowed`
- U03 fmt backend current source:
  - `closeout-author-u03-fmt-build-release-r2.json`: PASS
  - `closeout-author-u03-fmt-ctest-release-r2.json`: PASS, required `backend=fmt`, `backend_runtime_error=1`, `logger_error_handler_calls=1`, `active_macro_calls=0`, `active_macro_calls=1`
  - `closeout-author-u03-fmt-build-debug-r2.json`: PASS
  - `closeout-author-u03-fmt-ctest-debug-r2.json`: PASS with the same required output
- U03 std backend current source:
  - `closeout-author-u03-std-build-release-r2.json`: PASS
  - `closeout-author-u03-std-ctest-release-r2.json`: PASS, required `backend=std`, `backend_runtime_error=1`, `logger_error_handler_calls=1`, `active_macro_calls=0`, `active_macro_calls=1`
  - `closeout-author-u03-std-build-debug-r2.json`: PASS
  - `closeout-author-u03-std-ctest-debug-r2.json`: PASS with the same required output
- Source freeze for reviewed files:
  - `closeout-author-source-sha-r2.json`: PASS

Static R2 check:

- `rg` confirms `std::cref`, `borrowed_text`, `owned_text`, backend-specific `catch (const std::format_error&)`, backend-specific `catch (const fmt::format_error&)`.
- `rg` finds no remaining `catch (const std::exception&)` in the reviewed U02/U03 observation files.

No remaining CRITICAL/HIGH/MEDIUM issues in this slice. LSP/ast-grep remained unavailable in this environment; per task instruction, this closeout uses real MSVC/CTest plus source-level grep evidence.
