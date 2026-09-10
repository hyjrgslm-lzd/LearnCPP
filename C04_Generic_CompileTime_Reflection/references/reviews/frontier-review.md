# C04 frontier independent review

Date: 2026-09-09
Reviewer: non-author code review lane
Verdict: REQUEST CHANGES

Reviewed version:

- Repository HEAD: `f261bea559d6722c31135fb2d3589be52fe958ed`
- Scope is untracked in this worktree: `C04_Generic_CompileTime_Reflection/`
- Frozen files reviewed: `chapters/13-reflection-model.md`, `chapters/14-splicing-generation.md`, `chapters/15-annotations-frontier.md`, `exercises/F01_frontier/**`
- Context files read: `CONTENT_REFACTORING_GUIDE.md`, `references/implementation-spec.md`, `references/standards-and-implementations.md`, `references/validation/reflection-author/frontier-run-summary.md`

External first-hand references checked:

- P2996R13 Reflection for C++26: https://wg21.link/p2996r13
- P3394R4 Annotations for Reflection: https://wg21.link/p3394r4
- P3491R3 define_static_{string,object,array}: https://wg21.link/p3491r3
- P4101R1 Consteval-only Values for C++26: https://wg21.link/p4101r1
- P3385R8 Attributes reflection: https://wg21.link/p3385r8

## Summary

Files reviewed: 18 frozen files plus 4 spec/evidence context files

Total issues: 6

By severity:

- CRITICAL: 0
- HIGH: 4
- MEDIUM: 2
- LOW: 0

Stage 1 spec compliance does not pass. The current MSVC run correctly proves the local no-`<meta>` SKIP path, but it does not prove the subject C++26/C++29 examples. Several probes would either fail on a supporting compiler or skip a real source/API failure as "capability missing". That violates `references/implementation-spec.md` line 52 and `chapters/13-reflection-model.md` line 81: when a toolchain claims capability, source compile/link/run failures must be FAIL, not SKIP.

Stage 2 static scan found no hardcoded secret pattern in the frozen files. `lsp_diagnostics` and `ast_grep_search` are not available in this reviewer environment, so no LSP-clean approval is claimed.

## Issues

### [HIGH] Reflection and annotation probes persist `std::vector<meta::info>` in `constexpr` variables

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/reflection_query_probe.cpp:37`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/reflection_query_probe.cpp:42`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/annotations_probe.cpp:41`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/annotations_probe.cpp:42`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:36`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:37`

Trigger: build with an implementation that provides `<meta>`, `^^`, and the corresponding feature macro or forced probe macro.

Root cause: `nonstatic_data_members_of` and `annotations_of` return `std::vector<std::meta::info>`, but the probes store those vectors as `constexpr auto`. The course text itself teaches the opposite boundary at `chapters/13-reflection-model.md:38` and `chapters/14-splicing-generation.md:41-50`: query vectors are constant-evaluation temporaries and should be consumed transiently or promoted with `std::define_static_array`. P4101R1 also calls out `std::vector<std::meta::info>` as technically ill-formed under the consteval-only type model.

Impact: a compiler that actually reaches these branches can fail the complete subject code. Because CMake also uses the same bad pattern as the annotation capability probe, that failure can be converted into `F01_HAS_ANNOTATIONS=FALSE`, after which the runtime test returns SKIP instead of exposing the source bug.

Minimal fix: do not bind these query results to `constexpr std::vector` variables. For scalar checks, put the vector inside a `consteval` helper and return only `bool`, `size_t`, or other ordinary values. For iteration/splicing examples, feed the query directly through `std::define_static_array(...)` at the `template for` site. Keep a separate positive control for header/syntax capability.

### [HIGH] CMake capability detection uses full subject probes as the feature gate

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:29`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:40`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:42`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:56`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:58`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:72`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:105`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:113`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:118`

Trigger: any implementation where the basic feature exists but the course's full subject code has a syntax/API/semantic error.

Root cause: `check_cxx_source_compiles` is being used as both the capability probe and the course example validation. Its result controls whether the real target gets the macro that compiles the body. A failed subject compile therefore disables the body and makes the test return 77.

Impact: this is a masking fallback. It directly violates the F01 stop condition in `exercises/F01_frontier/README.md:7-9`: capability present plus source failure should be FAIL. Current wiring can report SKIP and make `ctest` print "100% tests passed" even though the subject example was never compiled.

Minimal fix: split each feature into a tiny positive capability check and a real subject target. If the tiny check fails, SKIP is valid. If it passes, compile/link/run the full source unconditionally and let any failure surface as build/test failure. Store the raw positive-control and subject-failure evidence separately.

### [HIGH] P4101 DR probe does not test P4101 behavior and uses the wrong marker

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/consteval_only_values_probe.cpp:10`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/consteval_only_values_probe.cpp:28`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/consteval_only_values_probe.cpp:31`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/answers.md:17`

Trigger: an implementation with P2996 reflection support but without P4101R1 consteval-only value semantics, or with only partial DR support.

Root cause: `__cpp_impl_reflection >= 202603L` is treated as the DR marker, but P4101R1 proposes changing the consteval model and bumping `__cpp_consteval`, not `__cpp_impl_reflection`. The executable only checks `std::meta::info{} != ^^int`, which is a basic reflection smoke check. It does not exercise the old-vs-new semantic differences: non-null reflection values escaping into non-constexpr runtime objects, null reflection treatment, pointer/reference immediate-object rules, or rejection of leaks.

Impact: this can print `PASS P4101R1 consteval-only value smoke check` on an implementation that has not implemented the P4101 behavior. That turns an unverified DR into a nominal PASS, which the user explicitly disallowed.

Minimal fix: rename the current check as a core reflection smoke check or remove it from the P4101 row. For P4101, add explicit compile-positive and compile-negative cases that distinguish the value model from the older type model, for example null `std::meta::info` runtime allowance if that model is selected, rejection of `auto d = ^^int`, rejection of leaking a non-null reflection into a non-constexpr object, and allowance of the `constexpr std::span<std::meta::info const>` workaround discussed by P4101R1. Until that exists, mark P4101 as "DR unverified".

### [HIGH] P3385 attributes reflection probe uses a nonexistent API path and lacks macro detection

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/attributes_reflection_proposal_probe.cpp:3`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/attributes_reflection_proposal_probe.cpp:20`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:122`

Trigger: compiler or experiment defines the P3385R8 implementation marker, or reviewer forces `C04_FORCE_ATTRIBUTES_REFLECTION_PROBE`.

Root cause: P3385R8 proposes `<meta>` facilities such as `std::meta::attributes_of`, `std::meta::has_attribute`, `std::meta::is_attribute`, and the macro `__cpp_impl_reflection_attributes`. The current probe includes no `<meta>`, never checks that macro, and calls `attribute_count(^^deprecated_function)`, which is not the proposed API.

Impact: the proposal experiment cannot become a valid PASS path. With a real implementation marker it would still SKIP unless manually forced; when forced it compiles the wrong API. This is not just "unimplemented compiler"; the subject source is not aligned to the stated proposal.

Minimal fix: gate on `__cpp_impl_reflection_attributes` or an explicitly documented force macro, include `<meta>`, and test the actual proposed API: reflect the function/entity, call `std::meta::attributes_of(...)` or `std::meta::has_attribute(...)`, and compare against a reflected standard attribute such as `^^[[deprecated]]` if the implementation supports it. Keep P3385 status as proposal/unverified unless this branch really compiles.

### [MEDIUM] GNU reflection try-compiles omit the required `-freflection` option

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:28`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:83`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/CMakeLists.txt:84`

Trigger: GCC reflection implementation where `<meta>`/reflection support requires `-freflection`.

Root cause: real targets get `-freflection` in `f01_configure_frontier`, but `check_cxx_source_compiles` only uses `${F01_CXX26_FLAG}` / `${F01_CXX29_FLAG}` through `CMAKE_REQUIRED_FLAGS`. The capability check and the target compile do not use the same feature flags.

Impact: a GCC implementation can be misclassified as missing annotations or reflection-adjacent support during configure, then the target takes the SKIP path even though the feature might be available with the same flags used for normal targets.

Minimal fix: build a single helper for required probe flags and target flags, or append `-freflection` to `CMAKE_REQUIRED_FLAGS` for GNU reflection/annotation try-compiles. Record a basic positive control first, then compile the full subject separately.

### [MEDIUM] Fold constraint probe compiles the gated subject even when body is disabled

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/fold_constraints_probe.cpp:4`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/fold_constraints_probe.cpp:7`

File: `C04_Generic_CompileTime_Reflection/exercises/F01_frontier/probes/fold_constraints_probe.cpp:11`

Trigger: normal configure where `C04_HAS_FOLD_EXPANDED_CONSTRAINTS` is not defined and `C04_TRY_FOLD_CONSTRAINTS` is defaulted to `0`.

Root cause: line 11 checks `defined(C04_TRY_FOLD_CONSTRAINTS)` instead of the macro value. Because lines 7-9 always define the macro, the overload-ranking subject is compiled even when the runtime body reports `body=0`.

Impact: the current subject happens to be parseable C++23, so MSVC still builds and returns SKIP. The guard is still wrong: if the disabled subject later needs C++26-only syntax, an unsupported compiler can fail the build instead of returning 77. It also makes the printed `body=0` misleading because some body support code was compiled.

Minimal fix: change the preprocessor guard to `#if C04_TRY_FOLD_CONSTRAINTS`, matching the later runtime branch at line 43.

## Validation Performed

Local MSVC validation tree:

- Configure: `cmake -S C04_Generic_CompileTime_Reflection/exercises -B C04_Generic_CompileTime_Reflection/references/validation/frontier-review/build-msvc-frontier -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DGENERIC_STUDY_BUILD_REFERENCE=ON -DGENERIC_STUDY_TEST_STUDENTS=OFF -DGENERIC_STUDY_ENABLE_ASAN=OFF -DGENERIC_STUDY_ENABLE_FRONTIER=ON -DGENERIC_STUDY_ENABLE_UNSAFE_DEMOS=OFF`
- Result: PASS configure. `F01_HAS_ANNOTATIONS`, `F01_HAS_FOLD_EXPANDED_CONSTRAINTS`, and `F01_HAS_TEMPLATE_NAME_PACK_INDEXING` all failed their try-compile checks.
- Build: `cmake --build C04_Generic_CompileTime_Reflection/references/validation/frontier-review/build-msvc-frontier --config Release`
- Result: PASS build. All 12 `F01_*` executables were generated.
- Test: `ctest --test-dir C04_Generic_CompileTime_Reflection/references/validation/frontier-review/build-msvc-frontier -C Release -R "^F01_" --output-on-failure`
- Result: PASS process exit, but all 12 tests were SKIP.
- Direct executable run: all 12 `F01_*` executables returned exit 77. `F01_fold_constraints` printed `macro=201603 body=0`; reflection/meta probes printed `header=0`.

This validates only the local no-capability path and runner behavior. It does not validate the enabled C++26/C++29 subject bodies.

Unavailable required checks:

- `lsp_diagnostics`: tool not available in this reviewer environment.
- `ast_grep_search`: tool not available in this reviewer environment; substituted targeted `rg` scans for secrets, empty catch, force macros, and suspicious fallback/SKIP paths.

## File Fingerprints

SHA256 fingerprints of reviewed frozen files:

- `chapters/13-reflection-model.md`: `39E78F577E1DA335F636AFC63E1FF4E0D910D57B93345128114B14CF5F7FEDEE`
- `chapters/14-splicing-generation.md`: `3C275ACD5AFF52E05BF7108937E97CE9B6627AC18C1C3270B656C55E70AB0255`
- `chapters/15-annotations-frontier.md`: `CA0955EE59CAB46B2E7B727503EE6859A22DC1F83EA4A964DB806E84C1CC7FA8`
- `exercises/F01_frontier/CMakeLists.txt`: `9F13E1B08DC04853DD707D7C6F0257BE3EA25C64D0F0DC667CAE331973B10D66`
- `exercises/F01_frontier/README.md`: `D772524443E77EF878FFBB111BFB256E64F86E299473AA97016597F4D80EDAEB`
- `exercises/F01_frontier/answers.md`: `91330C84FEC0BBD07DD2E239B54A4B3D6566ABE6151BA65E520F44511E040EDF`
- `exercises/F01_frontier/probes/annotations_probe.cpp`: `4B58B2C2513E41324F57A93E96735C776869A8B8D79A41439B3832FED6275087`
- `exercises/F01_frontier/probes/attributes_reflection_proposal_probe.cpp`: `71AE96558B9AE1C5BA9E47AA3472124C9D7DCF398B860F3FC24B3F127E36F169`
- `exercises/F01_frontier/probes/c29_template_name_pack_indexing_probe.cpp`: `29C86AC81750495172A97F4C4AE1458DA78CEF6B398BFC92E17D03F55D4CEA40`
- `exercises/F01_frontier/probes/consteval_only_values_probe.cpp`: `B29A321D8DCDC0EA6A08A120CC2A446A70E06EE0F50A0A955765A618A1493FE9`
- `exercises/F01_frontier/probes/constexpr_exceptions_probe.cpp`: `5F32D804D6AB18436E490FDF89547BA21240964BEB9FEACA0CCD80D33E45F65D`
- `exercises/F01_frontier/probes/define_aggregate_probe.cpp`: `B656A344C0A97A1ECE01D89E0407152E06FD796F1E9BBDA332B7936AB7DAA505`
- `exercises/F01_frontier/probes/define_static_probe.cpp`: `A33F3CBBCF70FC36224B3687B619B15C91B36E3BA3764FCDD75ED1EF4CC92957`
- `exercises/F01_frontier/probes/expansion_statement_probe.cpp`: `E07654E985246C5444872BDD52803A63BD27A14C3F9C77FE42B25FC9D5DAAEE8`
- `exercises/F01_frontier/probes/fold_constraints_probe.cpp`: `94FD7EB4F1B5A986CCE1CE529C9D9942DFB47BF43941221A5BBDF50D6529B5C2`
- `exercises/F01_frontier/probes/pack_indexing_probe.cpp`: `5D50CBD3144DD3069B22105170428E1A55FE80D640A15E4D717EAF367D9F603F`
- `exercises/F01_frontier/probes/reflection_query_probe.cpp`: `E9F8FCACFC7227E47FEA953DF57699502A86213341E62ECA805079E9A83C2551`
- `exercises/F01_frontier/probes/splicing_generation_probe.cpp`: `58651BF46449272323A867A695FA1CA079A5769A25B269933D9F53EBB09DD5D3`

## Recommendation

REQUEST CHANGES.

Fix the masking capability-gate structure first. Then repair the three incorrect subject probes: vector lifetime in reflection/annotation checks, P4101 DR semantics, and P3385 API/macro use. After that, rerun the same local MSVC no-capability path and record that it remains SKIP-only, with the enabled subject paths still explicitly marked unverified unless a real supporting compiler is available.
