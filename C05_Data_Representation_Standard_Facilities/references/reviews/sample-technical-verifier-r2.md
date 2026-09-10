# C05 sample technical review r2

## Verdict

APPROVE.

The r1 blocker is fixed. The L14 bad variant, expected diagnostic, README, and chapter 14 now all describe the same single mechanism: the bad implementation commits `cursor` after the length header, before payload validation succeeds.

## Evidence

- `validation/bad/bounded_field.hpp` hash: `20DA195F708113F967D2BB0013316038A96945928CB11164D3951DE940DEFE4F`.
- `CMakeLists.txt` hash: `070E8B15A52AAB0828649F6E7C98B2D7856687263E8687900579426B4A94E893`.
- `README.md` hash: `F004F1F48C50799995624E9E8A165B1B3E83EDC28FB742D098A870FA087FF219`.
- `chapters/14-bounded-binary-fields.md` hash: `F39727CEF8B672F96365983E1700EFB65AE3F9BCE0E40252317D2D972FD3B6D1`.
- Direct bad run: `build\review-sample-r2\leaf-L14\Release\L14_binary_fields_validation_bad.exe` exited 1 with `check failed: truncated payload keeps cursor`.
- Independent L14 leaf run: `ctest --test-dir build/review-sample-r2/leaf-L14 -C Release --output-on-failure` exited 0, 3/3 passed.
- Author state-fix evidence reviewed: Release core 13/13, Debug core 13/13, L14 leaf 3/3 all passed.

## Gaps

- This r2 pass intentionally did not rerun the full Unicode/bytes oracle; prior r1 oracle evidence is inherited because core `types/bytes/utf` sources were not part of this repair.

## Risks

- Existing CMake wrapper parallel-build oddity from r1 remains an environment/tool-wrapper risk, not a L14 state-fix blocker.
