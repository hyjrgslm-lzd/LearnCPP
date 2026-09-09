# C02 independent review — root public docs r1

## Verdict

REQUEST_CHANGES for this bounded root-docs slice.

One current status statement in `quality-report.md` is stale relative to the frozen independent verifier report for Student wiring / `record_process` cwd. Other reviewed route, prerequisite, boundary, preset, link, standards/probe, and known-history-gap statements were consistent with the current implementation-stage scope.

## Scope reviewed

- `Core_Study/README.md`
- `Core_Study/exercises/BUILD_GUIDE.md`
- `Core_Study/references/implementation-spec.md`
- `Core_Study/references/coverage.md`
- `Core_Study/references/quality-report.md`
- `Core_Study/references/standards-and-implementations.md`
- Supporting status source: `Core_Study/references/validation/reviews/verifier-student-wiring-r1.md`
- Supporting preset source: `Core_Study/exercises/CMakePresets.json`

This review treats the docs as implementation-stage docs. Links to chapters/exercises not yet fully delivered are not counted as final navigation blockers. Final full-course delivery and navigation review still remain required after all topic groups freeze.

## Source binding

Hashes are saved in `c02-independent-evidence/root-docs-r1/source-hashes.json`.

Key hashes:

- `Core_Study/README.md`: `E5582F9C32D9537EAD50088EAF5228BA0C17344CC0DDFA3174887C1AF13DCC99`
- `Core_Study/exercises/BUILD_GUIDE.md`: `FE6E2CEB39F87494370B4CB71CF3E1BFBDCE078C674ECCEC94DC001A46B8AD02`
- `Core_Study/references/implementation-spec.md`: `2EB445384AE32FD123C95ABC455D7921E347CB625B252AF9BD5776023A4FC24A`
- `Core_Study/references/coverage.md`: `9775151F64AFBA968A5B489376A1A369E00D48470ABF05F6AA51F8F6D5598DC2`
- `Core_Study/references/quality-report.md`: `266AB8F3167F1E511C983A43257A201C53B9E8781E0A880A408C0496ECCDE251`
- `Core_Study/references/standards-and-implementations.md`: `C7F9673507A908083965DCA8426C85A3834CAB0BD26FF9FD6EFFBE794ED12543`
- `Core_Study/exercises/CMakePresets.json`: `F7BFB7A45FE955004BFFF3B5C629942D21242FBD7AA326D0C1CFD279CBD64E5F`
- `Core_Study/references/validation/reviews/verifier-student-wiring-r1.md`: `9C9FCA5F96A76A7219E530BEEA4DE1CB3F7590A965843194F37802D616AB976C`

## Evidence

- `git status --short -- Core_Study` showed `Core_Study/` is currently untracked in this checkout, so there is no useful tracked diff for these docs.
- `cmake --list-presets -S Core_Study/exercises` listed configure presets `verify-core`, `verify-debug`, `student`, `asan`, `frontier`.
- From `Core_Study/exercises`, `cmake --build --list-presets` and `ctest --list-presets` both listed `verify-core`, `verify-debug`, `student`, `asan`, `frontier`. Saved in `c02-independent-evidence/root-docs-r1/preset-check.txt`.
- Local Markdown link extraction for the six docs found no missing local link targets in the current workspace. Saved in `c02-independent-evidence/root-docs-r1/local-link-check.json`. This is not a final delivery approval for not-yet-frozen chapters.
- `verifier-student-wiring-r1.md:3-7` says `APPROVE for this slice` and defines scope as `audit_student.py`, `student-wiring/**`, `record_process.py` `cwd`, and public include-order helper binding, limited to declared Reference-path / Student-target wiring checks.
- `quality-report.md:20` correctly preserves the L07 early evidence gap: it says missing early intermediate JSON cannot be forged or replaced, and the final quality conclusion must rely on latest frozen validation.
- `README.md:36-38`, `implementation-spec.md:77-92`, `coverage.md:3`, and `quality-report.md:3,22` correctly state implementation-stage evidence boundaries, unsupported/frontier/platform gaps, and that file existence/SKIP/green aggregate does not prove course completion.
- `standards-and-implementations.md:29-41` separates fixed standard/probe records from final course validation and says local probes do not auto-cover later changes.

## Stage 1 — spec/status compliance

Route and prerequisite responsibilities are coherent for this root-doc slice:

- `README.md:5,30` requires C01 prerequisite and says Core_Study owns the object model/reference/lifetime foundation before C03/C04/C06/C07.
- `implementation-spec.md:34-42` requires ground-up chapter/Part teaching, hard prerequisite checks, sample gate before parallel expansion, and isolated author/reviewer/verifier roles.
- `coverage.md:7-22` distinguishes approved sample slices from pending topic groups. Pending rows are acceptable under the stated implementation-stage scope.
- `BUILD_GUIDE.md:15-45` commands match the actual preset names in `CMakePresets.json`; Reference default ON and Student default OFF match `CMakePresets.json` cache variables.
- Historical evidence limitations are explicit; I did not find a claim that all early records exist.

Blocking mismatch: current `quality-report.md` does not reflect the frozen Student-wiring independent approval.

## Stage 2 — quality/security/static checks

These are Markdown/status docs plus CMake preset metadata, not compile units. No `lsp_diagnostics` tool is available in this lane and there are no source files modified by this slice. I used source reading, local link extraction, preset enumeration, and status-line grep instead.

No security issue, credential exposure, broad fallback/workaround masking, or external-system action was found in the reviewed docs.

## Issues

[MEDIUM] Current quality status underreports the independently approved Student wiring / cwd slice  
File: `Core_Study/references/quality-report.md:10-11`  
Issue: Line 10 says the later `record_process` cwd field and Student wiring tool still need separate review, and line 11 records only self-check evidence. That is stale against `Core_Study/references/validation/reviews/verifier-student-wiring-r1.md:3-7`, which already gives independent `APPROVE` for `audit_student.py`, `student-wiring/**`, `record_process.py` `cwd`, and include-order helper binding. As written, the quality report gives readers the wrong current gate state and blurs the distinction between self-check evidence and non-author approval.  
Fix: Update lines 10-11 to bind the current approval explicitly. Suggested content shape: keep `verifier-c02-public-tools-asan-r1.md` for the older public tools/ASan slice; add/link `validation/reviews/verifier-student-wiring-r1.md` for `record_process` cwd + Student wiring independent APPROVE; preserve the verifier's stated scope limit that this proves declared Reference-path/Student-target wiring checks, not arbitrary malicious C++ sandboxing or plagiarism detection.

## Recommendation

REQUEST_CHANGES.

After that status correction, this root public-doc slice can be rechecked narrowly. Full-course/final-navigation approval must still wait for complete topic-group delivery and frozen evidence.
