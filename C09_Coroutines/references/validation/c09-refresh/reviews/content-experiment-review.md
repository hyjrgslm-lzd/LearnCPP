# C09 content/experiment non-author review

Reviewer: verifier, non-author.  
Date: 2026-09-11.  
Workspace HEAD: `75028ff2d312a4f1047e7bafd6cf4101f771a2ec`.

## Verdict

APPROVE for the H/content/student-checker slice and F3 final evidence reviewed here.

R6 复验结论：H1/H2/H3 的常量、fake trace、旧 public `op.counters`、H2 旧 `task_trace*` 签名、H3 receiver-slot、H3 double-completion 代表性 bad 均已关闭。RefON 下 H1/H2/H3 `good` 与 `bad_constant_rejected` 六项通过；StudentRefOFF 不注册 H good/bad/reference/answer 变体；默认 Student H1/H2/H3 有限失败，H2 probe 77/SKIP。当前未发现 H1/H2/H3 Student checker 剩余实际阻断。

F3 final evidence R7 复验结论：`performance/final/analysis.md` 与 `summary.json` 已生成；n32/n256 统计、artifact SHA、IR/asm 归档、Rpass 空输出、`local_consume`/`escaped_consume` 无动态分配、独立 `range_values` 仍 `_Znwm(48)`、`bench_ns` 循环内间接调用均已核对。采样前后 Windows 进程快照非空，本报告只认可“本任务未启动并行构建”的表述，不认可“整机 quiet/no-parallel”。最终 delivery manifest/quality 若仍有后续更正，需按最终文件再核。

## Evidence

- `content_r6_configure_refon_light-20260911T050259333275Z.json` — fresh RefON light configure PASS；MSVC 19.51；H2 standard-library task probe仍未误报。
- `content_r6_build_h_variants-20260911T050330677314Z.json` — H1/H2/H3 good + bad_constant targets build PASS。
- `content_r6_ctest_h_variants-20260911T050333979170Z.json` — H1/H2/H3 good + bad_constant_rejected 共 6 项 CTest PASS。
- `content_r6_configure_student_light-20260911T050545513287Z.json` — fresh StudentRefOFF light configure PASS。
- `content_r6_ctest_list_student_h-20260911T050545707112Z.json` — StudentRefOFF H list contains only `H1_as_awaitable`, `H2_std_execution_task`, `H2_std_task_probe`, `H3_bidirectional_bridge`.
- `content_r6_student_h_list_no_variants_exact-20260911T050545907608Z.json` — exact grep against H Student list returns exit 1 as expected; no H good/bad/reference/answer entry.
- `content_r6_build_h_student_and_probe-20260911T050558781604Z.json` — H1/H2/H3 starter + H2 probe build PASS。
- `content_r6_ctest_h_student_expected_failures-20260911T050600700398Z.json` — default Student H1/H2/H3 finite-fail with `student check failed`; H2 probe skipped by 77。
- `content_r5_h1_legal_no_start_rejected-20260911T045440695754Z.json` — H1 legal no-start fake returns correct values but checker-private counters stay `0/0`; checker rejects。
- `content_r5_h2_legal_constant_rejected-20260911T045440818702Z.json` — H2 new-signature constant fake returns correct observations but checker-private counters stay `0/0/0/0`; checker rejects。
- `content_r5_h3_bridge_no_start_rejected-20260911T045440950321Z.json` — H3 bridge no-start fake gets values/body counters but bridge fixture counters stay `0/0`; checker rejects。
- `content_r5_h1_old_trace_access_compile_negative-20260911T045606433143Z.json` — old H1 `.trace` fake fails to compile: no `trace` member。
- `content_r5_h2_old_trace_signature_compile_negative-20260911T045609249508Z.json` — old H2 `task_trace*` signature fake fails to compile: no matching `run_task_observations` call。
- `content_r5_h3_old_trace_access_compile_negative-20260911T045611913185Z.json` — old H3 `.trace` fake fails to compile: no `trace` member。
- `content_r5_h1_op_counters_attack_compile_negative-20260911T045542758338Z.json` — H1 `op.counters` direct access fails to compile with private access diagnostic。
- `content_r5_h3_op_counters_attack_compile_negative-20260911T045545589133Z.json` — H3 `op.counters` direct access fails to compile with private access diagnostic。
- `content_r6_h3_receiver_slot_attack_compile_negative_exact-20260911T050408720357Z.json` — old H3 receiver-slot direct write attack fails to compile: `int_receiver` no longer has public `value` member。
- `content_r6_h3_double_setvalue_configure-20260911T050452432179Z.json`, `content_r6_h3_double_setvalue_build-20260911T050456572099Z.json`, `content_r6_h3_double_setvalue_rejected-20260911T050457135527Z.json` — H3 copied-good double `ex::set_value` attack builds but runtime rejects with `student check failed: sender must complete exactly once`。
- `content_r6_coverage_integrity_unique-20260911T050616734131Z.json` — coverage integrity PASS for 37 unique expected unit IDs, 37 exercise directories, and 37 README files。
- `content_r7_verify_f3_final-20260911T052047135828Z.json` — supervised verification PASS: recomputed `summary.json` n32/n256 med/min/max and sink values; verified artifact SHA/bytes and archived IR/asm hashes; checked `local_consume` and `escaped_consume` function bodies have no `_Znwm` and are loop-reduced; checked standalone `range_values` still calls `_Znwm(i64 48)`; checked `bench_ns` keeps the indirect consumer call inside the measurement loop; checked `-Rpass=coroutine-elide` emitted no stdout/stderr; checked Windows build-process before/after snapshots are non-empty, so whole-machine quiet is not proven.
- `C09_Coroutines/references/validation/c09-refresh/performance/final/analysis.md` — final F3 wording matches evidence: no pass-specific elision claim, no module-wide allocation inference, no claim that B's escaped global address write blocks HALO, and timing difference is descriptive only.
- `C09_Coroutines/references/validation/c09-refresh/performance/final/summary.json` — records source/sampler/executable/IR/asm SHA-256, sample records, iterations, expected sinks, and recomputed statistics for n32/n256.
- `C09_Coroutines/exercises/H3_bidirectional_bridge/checks/main.cpp:52-98` — current `int_receiver` keeps state/value/error/completion channel private; only completion CPO friends mutate it; `run_inline_sender` checks connect-before-start has 0 completions, start has exactly 1, and value/error/stopped are mutually exclusive。
- `C09_Coroutines/exercises/H1_as_awaitable/checks/fixture.hpp:26-75` and `C09_Coroutines/exercises/H3_bidirectional_bridge/checks/fixture.hpp:26-74` — traced sender state and `op_state` members are private; Student cannot mutate counters through legal member access。
- `C09_Coroutines/exercises/H2_std_execution_task/checks/main.cpp:11-62` — stopped/token counters live in checker-owned task bodies; Student receives `StoppedTask` and `TokenTask`, not writable trace pointers。

## Content and coverage checks

- `C09_Coroutines/references/coverage.md:14-54` contains the 37-unit table: P1/P2, A1-A3, B1-B3, C1-C3, Capstone1, D1-D3, E1-E3, F1-F3, G1-G3, H1-H3, I1-I5, J1-J3, Capstone4, Capstone5.
- `C09_Coroutines/references/coverage.md:56-85` gives the reverse audit chain per unit group and separates observation/starter/reference/capability states.
- `C09_Coroutines/README.md:5-17` states 34 exercises + 3 projects = 37 units and separates Student TODO, Reference, observation, and independent good/bad evidence.
- `C09_Coroutines/exercises/BUILD_GUIDE.md:118-128` documents Student route, Reference-off behavior, H2 probe 77/SKIP, runtime_tests Reference-off boundary, and RPC answer/good separation.
- `C09_Coroutines/14-第三阶段结课-mini协程库实现.md:19-34` and `:280-292` explain mini runtime contracts and Student-vs-Reference boundaries.
- `C09_Coroutines/exercises/Capstone5_mini_corolib/README.md:130-148` lists actual Student parts/checks and states unfinished tests fail normally, without WILL_FAIL.
- `C09_Coroutines/references/validation/c09-refresh/reviews/rpc-review.md` and `mini-student-review.md` remain separate APPROVE reviews for RPC and mini good/bad; I did not repeat them.

## Gaps

- Final delivery manifest/quality were not terminal-reviewed in this R7 pass if parent modifies them after `analysis.md`/`summary.json`; rerun manifest/quality readback on the final files.
- I3/I4 dependency checks were not rerun in this Windows slice; no new I3/I4 blocker was proven here.
- I did not test malicious preprocessor hacks such as redefining `private` or memory-unsafe overwrites; those are outside the requested representative legal Student-interface attack scope.

## Stop condition

For this review slice, stop condition is met: H1/H2/H3 checker hardening has direct good/bad/ref-off/default-student evidence, and the previous verifier-discovered bypasses are closed. Final C09 approval still needs manifest/quality terminal review only if those files changed after this F3 evidence review.

