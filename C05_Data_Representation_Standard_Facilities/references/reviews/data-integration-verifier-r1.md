# C05 data integration technical review r1

## Verdict

APPROVE.

This data/serialization/P1/public-build lane meets the requested gate on the current source bound in `references/validation/data-review-evidence-r1.md`.

## Evidence

- `ctest --test-dir build/data-review/leaf-L15 -C Release --output-on-failure` — L15 schema evolution leaf passed 3/3.
- `ctest --test-dir build/data-review/leaf-P1-r3 -C Release --output-on-failure` — P1 resource manifest leaf passed 3/3 after the cleanup-parent and empty-manifest invalid-zone fixes.
- `build\data-review\leaf-P1-r3\Release\P1_resource_manifest_validation_bad.exe` — exited 1 with `check failed: creates the package file`, proving the representative bad remains rejected and the old cleanup diagnostic no longer masks it.
- `build\data-review\probe-build\Release\c05_data_review_probe.exe` — printed `data probe passed`; reviewer probe covered duplicate record-id wire rejection, v1-reader/v2-note compatibility, and P1 no-resource-path-open behavior.
- `ctest --test-dir build/data-review/student -C Release --output-on-failure` — student ref-off configuration built and ran; 7 expected unfinished student failures were present with reference/good targets disabled.
- `references/validation/integration-ctest-r2.json` — root full Release integration evidence shows 33/33 passed; used as supporting evidence, not as the sole basis for approval.
- `rg -n "__has_include" ...` over scoped data/public-build files — no matches.

## Acceptance trace

- Encoder/decoder validation, bounds, offsets, duplicate/unknown fields, duplicate IDs, trailing bytes, old v1 reader, and independent golden consumption: covered by static inspection of `manifest.hpp`, `fixtures/golden.hpp`, `L15_schema_evolution/checks/schema_checks.cpp`, `provided/v1_reader.hpp`, plus fresh L15 leaf test and reviewer probe.
- P1 real file flow, no overwrite, cleanup-before-exit, unsafe `resource_path` not opened, invalid config/manifest no-output, bounded read/write, and expected error paths: covered by static inspection of `P1_resource_manifest` implementation/checker plus fresh P1 leaf test and reviewer probe.
- Public build gates: `StudySetup.cmake`, `ICU.cmake`, `CMakePresets.json`, and top-level CMake inspected; student ref-off build/run verified; scoped `__has_include` search clean.

## Gaps

- Full Unicode/bytes oracle was inherited from the earlier sample review because those core files did not change in this lane; final hashes are bound in the evidence file.
- ICU runtime execution and detailed time/config/path lesson behavior were outside this verifier lane; this review only checked public build isolation and dependency wiring relevant to data/P1.

## Risks

- No current blocking risk found in this lane.
