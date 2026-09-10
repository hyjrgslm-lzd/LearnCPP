# C04/C05 revision dependencies review

日期：2026-09-10

结论：REQUEST CHANGES。四个真实依赖 checkout 的 remote、HEAD、marker、license SHA、header marker 均已复核；C04 Mp11/Hana 的准备与 CMake 消费契约未发现阻断；C05 fmt 路径也能拒绝外部 `fmt::fmt` target 污染。但 C05 `c05_ensure_spdlog()` 对已存在的 `spdlog::spdlog` 直接早退，绕过 pinned source/static target 校验，违反 `revision-dependencies.md` 的“已有同名外部 target 会失败”契约。

## Reviewed scope

- `C04_Generic_CompileTime_Reflection/exercises/tools/prepare_meta_libraries.ps1`
- `C04_Generic_CompileTime_Reflection/exercises/cmake/MetaLibraries.cmake`
- `C04_Generic_CompileTime_Reflection/references/revision-dependencies.md`
- `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependencies-author-report.md`
- `C05_Data_Representation_Standard_Facilities/exercises/tools/prepare_format_libraries.ps1`
- `C05_Data_Representation_Standard_Facilities/exercises/cmake/FormatLibraries.cmake`
- `C05_Data_Representation_Standard_Facilities/references/revision-dependencies.md`
- `C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependencies-author-report.md`
- Dependency evidence directories:
  - `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependencies-meta-20260910-123124`
  - `C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependencies-format-20260910-123030`
  - `C04_Generic_CompileTime_Reflection/references/validation/revision-20260910/dependencies-cmake-20260910-123814`

I did not review newly written C04/C05 course chapters or lesson bodies in this slice.

## Verified contracts

- Fixed repositories and commits:
  - Boost.Mp11: `https://github.com/boostorg/mp11.git` at `b94b089d4ec83cd397f20958f34edf25bc3e06f4`
  - Boost.Hana: `https://github.com/boostorg/hana.git` at `bc49ee25638e59d977edff5737b4e6bf12c1e5ea`
  - fmt: `https://github.com/fmtlib/fmt.git` at `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f`
  - spdlog: `https://github.com/gabime/spdlog.git` at `79524ddd08a4ec981b7fea76afd08ee05f83755d`
- All four local checkout HEAD values match the pinned commits.
- All four local checkout remotes match the documented repositories.
- All four local checkout tracked/untracked status checks from direct `git status --porcelain=v1` were empty during review.
- All four marker license SHA values match freshly computed local file hashes.
- License marker content is real:
  - Mp11 `README.md` states Boost Software License, Version 1.0.
  - Hana `LICENSE.md` contains Boost Software License, Version 1.0.
  - fmt `LICENSE` contains the MIT-style grant text.
  - spdlog `LICENSE` contains MIT License text and notes fmt dependency.
- Prepare scripts constrain writes under their `_deps` root with `Assert-UnderRoot`, refuse non-empty non-checkout prefixes, verify remote/HEAD/dirty status, and write `.learncpp-dependency.json` plus `.learncpp-dependency.cmake`.
- CMake consumers perform offline local checks. The reviewed CMake modules call local `git rev-parse HEAD` and `git diff-index --quiet HEAD --`; they do not fetch or fallback to system libraries.
- OFF/ON semantics have evidence:
  - C04 meta positive configure/build/run: PASS.
  - C04 `GENERIC_STUDY_ENABLE_META_LIBS=OFF`: rejected.
  - C04 missing dependency: rejected.
  - C04 stale marker/HEAD mismatch: rejected.
  - C05 fmt backend configure/build/run: PASS; build output shows static fmt/spdlog objects/libraries from pinned source.
  - C05 std backend configure/build/run: PASS; build output shows static spdlog library and no fmt static build evidence in that path.
  - C05 bad backend: rejected.
  - C05 wrong fmt commit marker: rejected.
  - C05 external `fmt::fmt` target pollution: rejected.

## Issue

[MEDIUM] External `spdlog::spdlog` target pollution is accepted

File: `C05_Data_Representation_Standard_Facilities/exercises/cmake/FormatLibraries.cmake:94`

Issue: `c05_ensure_spdlog()` returns immediately when `spdlog::spdlog` already exists:

```cmake
if(TARGET spdlog::spdlog)
    return()
endif()
```

This happens before:

- validating `DATA_STUDY_SPDLOG_BACKEND`
- checking the pinned spdlog marker
- proving the target is a static library
- proving `SOURCE_DIR` equals the pinned checkout
- setting `C05_SPDLOG_BACKEND_SELECTED`

That contradicts `C05_Data_Representation_Standard_Facilities/references/revision-dependencies.md`, which says existing `fmt::fmt` or `spdlog::spdlog` targets must be from this course’s pinned static source and external predefined targets fail.

Reproduction artifact:

- Mock project: `C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-spdlog-pollution`
- Configure evidence: `C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-spdlog-pollution/configure.json`
- Build evidence: `C05_Data_Representation_Standard_Facilities/references/validation/revision-20260910/dependency-review-spdlog-pollution/build.json`

The mock defines an unrelated local static library and aliases it as `spdlog::spdlog`, then calls `c05_link_spdlog(app)`. Observed result:

- configure exit code `0`, verdict `PASS`
- build exit code `0`, verdict `PASS`
- build output emits `spdlog_external.lib`, proving the external alias was consumed

Expected result: configure should fail with a pinned-source error, as the existing `fmt::fmt` pollution check already does.

Fix: make `c05_ensure_spdlog()` mirror `c05_ensure_fmt()`:

1. Validate backend and load/verify the pinned spdlog marker before accepting any existing `spdlog::spdlog`.
2. If `TARGET spdlog::spdlog` already exists, call `c05_assert_pinned_static_target(spdlog::spdlog spdlog "${C05_SPDLOG_SOURCE_DIR}")` before returning.
3. Set `C05_SPDLOG_BACKEND_SELECTED` on the accepted early-return path.
4. Add a negative control for external `spdlog::spdlog` target pollution.

## Validation limits

No `lsp_diagnostics` or `ast_grep_search` tool is available in this child review environment. I used source inspection, author evidence JSON, direct local git/hash checks, targeted CMake evidence, and an isolated mock CMake project.

## Recommendation

REQUEST CHANGES until `c05_ensure_spdlog()` rejects external `spdlog::spdlog` target pollution. Do not treat existing fmt pollution evidence as covering spdlog; the current spdlog branch has a different early return.
