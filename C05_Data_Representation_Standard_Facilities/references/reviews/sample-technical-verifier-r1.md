# C05 sample technical review r1

## Verdict

BLOCK.

The scoped sample code passes fresh Release/Debug CTest, student initial state fails as intended, and the temporary oracle did not find a UTF/field/big-endian behavior defect. The blocker is evidence honesty: chapter 14 describes a bad variant and rejection diagnostic that no longer match the actual `validation/bad` implementation.

## Evidence

- `chapters/14-bounded-binary-fields.md:34` says L14 bad consumes the length header and leaves `cursor=4` on truncated payload.
- `chapters/14-bounded-binary-fields.md:36` says that bad is rejected by `truncated payload keeps cursor`.
- `exercises/L14_binary_fields/validation/bad/bounded_field.hpp:13-24` uses local `probe` and does not assign external `cursor` on truncated payload.
- `exercises/L14_binary_fields/validation/bad/bounded_field.hpp:30` assigns `cursor = probe` only after success.
- `exercises/L14_binary_fields/CMakeLists.txt:9` sets `BAD_DIAGNOSTIC "rejects invalid utf8"`.
- Direct bad run exited 1 with `check failed: rejects invalid utf8`.

## Validation

- Release core: configure ok, build ok with `--parallel 1`, CTest 13/13 passed.
- Debug core: build ok with `--parallel 1`, CTest 13/13 passed.
- Student/ref-off: build ok; CTest nonzero with L03/L06/L14 student failures and observation tests passing.
- L14 leaf: configure/build ok, CTest 3/3 passed.
- Independent oracle: built and ran; `oracle passed`.

## Required repair

Pick one truthful path:

- Change chapter 14 to describe the current bad variant: length/bounds/cursor mostly correct, missing strict UTF-8 validation, rejected by `rejects invalid utf8`.
- Or change `validation/bad` and `BAD_DIAGNOSTIC` back to the cursor-advance counterexample, then rerun L14 negative and core tests.

The first path is smaller and matches the current code.
