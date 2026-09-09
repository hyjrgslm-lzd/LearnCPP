# C02 author storage batch report

作者范围：章节 11-15，练习 `L11_layout`、`L12_storage`、`L13_aliasing`、`L14_ub`、`P1_object_buffer`，以及 `author-storage-probes` 能力探测证据。

## 交付内容

- `chapters/11-layout-and-representation.md`：布局、对齐、对象表示、数组边界、standard-layout、trivially-copyable、EBO/`[[no_unique_address]]`；区分标准保证、本机尺寸和 ABI 绑定。
- `chapters/12-storage-and-object-creation.md`：分配与构造、C++20 implicit object creation、`allocator<T>::allocate` 数组对象边界、placement new、`construct_at`/`destroy_at`、implicit-lifetime、`start_lifetime_as`、union 活跃成员。
- `chapters/13-aliasing-and-provenance.md`：合法类型访问、字节观察、`bit_cast`/`memcpy`、`launder`、透明替换、pointer provenance 与 C++29 DR 边界。
- `chapters/14-undefined-behavior-and-optimization.md`：UB、IFNDR、implementation-defined、unspecified、C++26 erroneous behavior、as-if、诊断工具边界和证据分类。
- `chapters/15-object-buffer.md`：冻结 API 的 `object_buffer<T>` 不变量、`reserve`/`push_back(T value)` 强保证顺序、失败注入矩阵、借用变化和下游回访。
- `exercises/L11_layout`、`L13_aliasing`、`L14_ub`：观察型实验，无 Student。
- `exercises/L12_storage`：`storage_slot<T>` Student 占位、Reference、checker、good/bad validation。
- `exercises/P1_object_buffer`：`object_buffer<T>` Student 占位、Reference、checker、good/bad validation、throwing-move-only 编译拒绝。

## 关键实现边界

- `object_buffer<T>` 保持 move-only；无 mutable view、iterator、复制容器、自定义 allocator 开关。
- `reserve` 全部成功后才提交新 storage/capacity；copy fallback 失败只清理新前缀。
- 扩容 `push_back(T value)` 先构造新尾，成功后迁移旧元素；新尾失败不销毁未构造对象，只释放新分配。
- 迁移失败只销毁成功的新前缀和已成功的新尾；旧状态保持不变。
- `T` 的 trait 前提和语义前提分开；throwing-move-only 编译拒绝。
- `view()` 是 `span<const T>`，不保活 owner；move 构造转交 allocation，move assignment 销毁目标旧数据。
- `malloc`/`operator new` 叙述按 C++20 implicit object creation 修正：分配函数不调用构造函数，但可能为 implicit-lifetime 类型隐式开始对象生命期；`allocator<T>::allocate(n)` 开始 `T[n]` 数组对象生命期，不开始元素生命期。

## 证据

Raw JSON 位于 `references/validation/author-storage/`：

- `l11-debug-ctest-r1.json`、`l11-release-ctest-r1.json`
- `l12-debug-nonstudent-ctest-r1.json`、`l12-release-nonstudent-ctest-r1.json`
- `l12-msvc-capability-configure-r1.json`、`l12-msvc-capability-build-r1.json`、`l12-msvc-capability-ctest-r1.json`
- `l12-msvc-start-lifetime-direct-r2.json`
- `l12-clang-start-lifetime-capability-r1.json`、`l12-clang-start-lifetime-direct-r2.json`
- `l13-debug-ctest-r1.json`、`l13-release-ctest-r1.json`
- `l14-debug-ctest-r1.json`、`l14-release-ctest-r1.json`
- `l14-msvc-default-build-r1.json`、`l14-msvc-default-ctest-r1.json`
- `l14-default-no-frontier-configure-r1.json`、`l14-default-no-frontier-tests-r1.json`
- `l14-clang-asan-safe-no-unsafe-tests-r1.json`
- `l14-msvc-frontier-configure-r1.json`、`l14-msvc-frontier-ctest-r1.json`
- `l14-msvc-frontier-configure-r2.json`、`l14-msvc-frontier-ctest-r3.json`
- `l14-clang-frontier-ctest-r1.json`、`l14-clang-frontier-ctest-r2.json`
- `l14-clang-unsafe-asan-r1.json`、`l14-clang-unsafe-asan-r2.json`
- `p1-debug-nonstudent-ctest-r1.json`、`p1-release-nonstudent-ctest-r1.json`
- `l12-student-placeholder-fails-r1.json`、`p1-student-placeholder-fails-r1.json`
- `l12-ref-off-build-r1.json`、`p1-ref-off-build-r1.json`
- `l12-asan-reference-runtime-fails-r1.json`、`p1-asan-reference-runtime-fails-r1.json`
- `p1-msvc-configure-r2.json`
- `p1-msvc-debug-build-r2.json`、`p1-msvc-debug-ctest-r2.json`
- `p1-msvc-release-build-r2.json`、`p1-msvc-release-ctest-r2.json`
- `p1-clang-asan-configure-build-reference-good-r2.json`
- `p1-clang-asan-reference-direct-r2.json`、`p1-clang-asan-good-direct-r2.json`
- `p1-reference-fixed-diff-r3.json`
- `doc-repair-asan-classification-r1.json`、`doc-repair-old-asan-claim-absent-r1.json`
- `doc-repair-standard-boundaries-r1.json`、`doc-repair-sensitive-scan-r1.json`
- `p1-r2-sensitive-scan-r4.json`
- `p1-r2-report-index-r1.json`
- `l14-frontier-status-r1/msvc/*.txt`、`l14-frontier-status-r1/clang/*.txt`
- `l12-l14-entry-text-scan-r1.json`
- `l12-l14-old-claims-absent-r1.json`
- `l12-l14-sensitive-scan-r1.json`

Capability probe archive:

- `references/validation/author-storage-probes/evidence/storage-probes-20260908-220803.json`
- Summary: `references/validation/author-storage-probes/results-20260908.md`

## 当前结果

- MSVC Debug/Release non-Student checks pass for all five leaves.
- L12/P1 Student placeholders fail quickly with checker diagnostics.
- L12/P1 reference-off builds succeed and do not include reference/validation targets in the generated solution.
- P1 public good passes; bad noop and early-commit variants are rejected; throwing-move-only compile target is rejected.
- Capability probe shows MSVC 19.51 + current STL supports `std::start_lifetime_as`; Clang 22.1.3 with current MSVC STL does not expose `__cpp_lib_start_lifetime_as` and the `start_lifetime_as` probe fails to compile.
- L12 now has a course target `L12_storage_start_lifetime_as`. MSVC Debug direct run prints `L12_start_lifetime_as OK macro=202207`; Clang 22.1.3 + current MSVC STL direct run prints `SKIP: __cpp_lib_start_lifetime_as not defined`. No fallback API is used to mark Clang PASS.
- L14 now has `CORE_STUDY_ENABLE_FRONTIER` consumers. MSVC/Clang frontier baseline compile probes pass. P2287R6 base/indirect member designator probe is SKIP on both current toolchains. P2748 return-temporary-reference probe: MSVC diagnoses then accepts, recorded as SKIP; Clang rejects, recorded as PASS for the rejection capability. P2953 defaulted-assignment restriction probe is SKIP on both current toolchains. Provenance/lifetime-end model compiles on both as compile-only review evidence, with no runtime support claim.
- L14 now has an unsafe ASan diagnostic target gated by both `CORE_STUDY_ENABLE_ASAN` and `CORE_STUDY_ENABLE_UNSAFE_DEMOS`. Clang ASan explicit run matches `heap-use-after-free` and `asan_uaf_probe.cpp`, and normal/default L14 remains safe.
- Default L14 configure without frontier/unsafe registers only `L14_ub_observation`; Clang ASan configure with unsafe off also lists only one safe test.
- P1 r2 Reference uses RAII rollback for allocation cleanup. MSVC Debug and Release non-student CTest both pass all 5 tests: Reference, validation good, bad noop rejection, bad early-commit rejection, and throwing-move-only compile rejection.
- P1 r2 Clang ASan config/build passes for Reference and validation good; direct exe runs with matching runtime `PATH` both print `P1_object_buffer_contract OK` and exit 0.

## ASan 证据纠正

`l12-asan-reference-runtime-fails-r1.json` and `p1-asan-reference-runtime-fails-r1.json` are preserved as raw intermediate evidence, but they are command invocation failures, not program runtime verdicts. Their recorded command shape contains a malformed `cmd /d /s /c` invocation with an empty command segment before the executable path, and stderr reports command/file-name syntax failure. The target executables did not start, so these JSON files must not be used as ASan PASS/FAIL evidence for L12 or P1.

Root later ran with a matching runtime `PATH` directly. That newer direct evidence reports L12 Reference PASS and a real P1 ASan access violation in `references/validation/asan/p1-reference-direct-r1.json`, around the CopyFallback `grow_and_append` rethrow path and `VCRUNTIME140.dll`. P1 implementation/checker was frozen while the dedicated ASan debugger lane isolated the cause; this report keeps the old wrapper JSON classified as command-startup failure, not runtime evidence.

The dedicated P1 ASan diagnosis then isolated the issue to the Clang 22.1.3 + Windows ASan + MSVC exception-handling rethrow path: independent `rethrow_only` reproduces the crash, while `throw_only` passes and non-ASan runs pass. This does not make C++ `throw;` generally invalid, and it does not by itself prove the P1 algorithm wrong.

The r2 Reference keeps the same strong-guarantee order and exception propagation, but replaces the two explicit catch-cleanup-`throw;` blocks with `std::unique_ptr` rollback guards whose deleters are `noexcept`. Cleanup now happens during stack unwinding without an explicit rethrow in those functions. The old root-cause evidence, minimal repro evidence, old failed JSON, and fixed-candidate evidence remain under `references/validation/debugger-p1-asan/`.

`validation/bad_early_commit` is intentionally left as a broken teaching implementation. It is still rejected by MSVC Debug/Release CTest through the expected-failure harness; it is not treated as a target for the Reference ASan workaround.

`p1-reference-fixed-diff-r3.json` records that the updated Reference source matches `debugger-p1-asan/fixed/object_buffer.hpp` by content and SHA256. Two intermediate sensitive-scan attempts, `p1-r2-sensitive-scan-r1.json` and `p1-r2-sensitive-scan-r3.json`, are preserved as failed command evidence: r1 used a wildcard that `rg` did not expand on Windows, and r3 scanned prior scan JSON files and matched the search expression itself. The corrected source/report scan is `p1-r2-sensitive-scan-r4.json`.

Two L14 intermediate JSON files are also preserved with corrected classification. `l14-msvc-frontier-ctest-r1.json` failed because the default observation executable had not been built before running all tests; the frontier probes in that run already executed, and the corrected frontier-only run is `l14-msvc-frontier-ctest-r2.json`, superseded by the cleaner post-P2748-fix `l14-msvc-frontier-ctest-r3.json`. `l14-clang-unsafe-asan-r1.json` failed only because outer `record_process` looked for ASan text in non-verbose CTest output; the CTest itself passed. The corrected verbose evidence is `l14-clang-unsafe-asan-r2.json`.

## 未做

- No root public files were edited.
- No 11-15 work was self-approved.
- No P1 implementation/checker edits were made after the ASan freeze notice.
- No new dependency, install, CI, commit, push, or unsafe UB runtime demo was added.

