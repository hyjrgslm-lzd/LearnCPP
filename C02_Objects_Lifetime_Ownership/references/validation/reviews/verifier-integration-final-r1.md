# C02 integration final verifier r1

## Verdict

- APPROVE

## Scope

验证候选快照 `Core_Study/references/validation/snapshots/candidate-r2` 与最终整合证据。未重跑全课，未触发已知弹窗风险场景；只做清单 SHA 复算、最终 JUnit/record JSON 解析、冻结脚本规则检查、Git 可见范围检查。

## Evidence

- `Core_Study/references/validation/snapshots/candidate-r2/summary.json` — 声明 `source_files=299`、`evidence_files=1266`、`source_manifest_sha256=6b1e21472131b13bdfe800d8b1c42eafca31d4e267012b2f5af5b8b768681880`、`evidence_manifest_sha256=594949caae1fee71ba366f775c8761b95ec9986490f0aa5c3ac5a3bc322675d8`。
- `Core_Study/references/validation/snapshots/candidate-r2/verifier-c02-manifest-rehash-r1.json` — 独立复算 `source.sha256` 与 `evidence.sha256`：299/1266 行均匹配，manifest SHA 均匹配 summary；malformed/missing/mismatch/duplicate 均为 0。
- `Core_Study/references/validation/snapshots/candidate-r2/verifier-c02-integration-evidence-r1.json` — JUnit/record 解析结果：
  - `integration/verify-debug-final-r2.xml`：40 tests，0 failures，0 errors，0 skipped。
  - `integration/verify-core-final-r2.xml`：40 tests，0 failures，0 errors，0 skipped。
  - `integration/final-asan-safe-r2.xml`：20 tests，0 failures，0 errors，1 skipped；skip 为 `L12_storage_start_lifetime_as`。
  - `integration/final-asan-unsafe-r2.xml`：1 test，0 failures，0 errors，0 skipped；`final-asan-r2.json` verdict/status 均 PASS。
  - `integration/frontier-final-r1.xml`：45 tests，0 failures，0 errors，3 skipped；skip 为 `L14_frontier_p2287_base_member_designator_compile`、`L14_frontier_p2748_return_temp_ref_compile`、`L14_frontier_p2953_defaulted_assignment_compile`。
  - `integration/frontier-probes-final-r3.xml`：5 tests，0 failures，0 errors，3 skipped；对应最新 probes，`frontier-probes-final-r3.json` verdict/status PASS。
  - `integration/student-initial-final-r1.xml`：9 tests，9 failures，0 errors，record `student-initial-final-r1.json` 进程 exit 8/status FAIL 但 verdict PASS，符合“初始学生目标预期失败”。
- `Core_Study/references/validation/snapshots/candidate-r2/verifier-c02-scope-git-wiring-r1.json` — 学生接线证据：`student-wiring-final-r1.json` verdict PASS、failures 0、students 9、Release、actual_include_count 1664；source hash 列表无 `reference` 路径命中，`student-build-trace-final-r1.json` exit 0/status PASS/verdict PASS，trace 中未抽到 Reference 依赖行。
- `Core_Study/references/validation/freeze_delivery.py` — 审查生成逻辑：输入来自 `git ls-files --cached --others --exclude-standard`；跳过 `/snapshots/`、`delivery-manifest.md`、review markdown；发现 `.exe/.dll/.obj/.lib/.pdb/.vcxproj/.slnx/.tlog` 会直接 `RuntimeError`；validation 下 `.cpp/.hpp/.h/.cmake/.py/.ps1/.cmd` 和 `CMakeLists.txt` 归 source，其余归 evidence；显式追加 3 个跨课程支持文件到 source。
- `Core_Study/references/validation/snapshots/candidate-r2/verifier-c02-scope-git-wiring-r1.json` — manifest 排除复核：candidate 清单中无 generated artifact、无 `/snapshots/`、无 review markdown、无 `delivery-manifest.md`、无本 verifier 新增候选输出；3 个显式外部支持文件都存在且在 source manifest。
- `git status --short --untracked-files=all` / `git ls-files --cached --others --exclude-standard` 解析结果已写入 `verifier-c02-scope-git-wiring-r1.json`：Git 可见文件 6140；visible generated artifacts 0；visible build-like paths 0；tracked changed files 正好 7 个：`README.md`、`LEARNCPP_GLOBAL_PLAN.md`、`Engineering_Study/README.md`、`Coroutine_Study/README.md`、`Concurrency_Study/README.md`、`Execution_Study/README.md`、`Ranges_Study/README.md`。
- 受前序阻断影响的存储/公共修复已在 `Core_Study/references/validation/reviews/verifier-storage-11-15-final-r2.md` 给出 APPROVE；当前 candidate-r2 包含对应 integration 证据：`clangcl-l14-link-fixed-r1.json`、`clangcl-l14-runtime-fixed-r1.json`、`l14-msvc-runtime-corrected-r1.json` 均 exit 0/status PASS/verdict PASS。

## Gaps

- 本轮按 root 指令未重新跑全课、未复现缺 DLL/弹窗场景；结论绑定现有 final evidence 与独立清单/证据解析。
- 最终文档状态翻转尚未发生；需要 root 生成 final 快照后再做快速 SHA 回读。

## Risks

- `student-wiring-final-r1.json` 证明的是声明学生目标的 CMake/codemodel/source literal/include trace 接线，不是任意恶意 C++ 沙箱，也不证明算法原创性。
- ASan/frontier skip 均有明确用例名；不能外推为对应未实现标准能力已运行通过。

## Final readback after status flip

- APPROVE remains valid after final status/doc flip.
- `Core_Study/references/validation/snapshots/final-r1/verifier-final-readback.json` — independent readback PASS: source 299, evidence 1266; source manifest SHA `4f68ffcd3df82868431d11acc23561ec3bbecccc1a637121ed7bdd57cbe941bd`; evidence manifest SHA `594949caae1fee71ba366f775c8761b95ec9986490f0aa5c3ac5a3bc322675d8`.
- Compared with `candidate-r2`, evidence manifest has 0 diffs. Source manifest has exactly 3 changed paths: `Core_Study/README.md`, `Core_Study/references/quality-report.md`, `LEARNCPP_GLOBAL_PLAN.md`.
- No course rebuild/rerun was performed for this readback, per root instruction; this is a manifest/SHA/scope closeout.
