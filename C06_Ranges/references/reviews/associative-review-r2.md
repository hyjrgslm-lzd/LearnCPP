# Foundation Associative Review r2

Verdict: APPROVE

Scope reviewed:

- `chapters/07-heap-hash-avl.md`
- `exercises/L08_avl_tree/README.md`
- `exercises/L08_avl_tree/CMakeLists.txt`
- `exercises/L08_avl_tree/checks/avl_checks.cpp`
- `exercises/L08_avl_tree/src/reference/avl_set.hpp`
- `exercises/L08_avl_tree/src/student/avl_set.hpp`
- `exercises/L08_avl_tree/validation/good/avl_set.hpp`
- `exercises/L08_avl_tree/validation/bad/avl_set.hpp`
- `exercises/L08_avl_tree/validation/bad_vector/avl_set.hpp`
- `exercises/L08_avl_tree/validation/bad_stale_height/avl_set.hpp`

Source baseline:

- HEAD: `f261bea559d6722c31135fb2d3589be52fe958ed`
- r2 source hashes and local review matrix: `C06_Ranges/validation/associative-review-r2-build-matrix.json`

Closed blockers:

1. Original BLOCK: AVL checker accepted a sorted-vector fake.
   - Status: closed.
   - Evidence: `checks/avl_checks.cpp:44` now calls `inspect_root()` and `inspect_subtree`; `inspect_subtree` independently checks cycle freedom, BST bounds, actual height, stored height, and balance factor at `checks/avl_checks.cpp:29-40`.
   - Evidence: `validation/bad_vector/avl_set.hpp:29` returns `nullptr` from `inspect_root()` while public values still look plausible; Debug/Release r2 tests reject it with `structural root is present`.
   - Evidence: `validation/bad_stale_height/avl_set.hpp:57` lies about stored height; Debug/Release r2 tests reject it with `stored height matches actual height`.
   - Evidence: `validation/good/avl_set.hpp:13-56` is now a true node-based AVL implementation with inspector accessors, not a sorted-vector oracle.

2. Original BLOCK: AVL teaching text did not support implementing the hidden Reference.
   - Status: closed.
   - Evidence: `chapters/07-heap-hash-avl.md:64-88` now defines `height(nullptr)=0`, recursive insert returning the repaired subtree root, duplicate behavior, refresh, balance factor, and checker-side structural recomputation.
   - Evidence: `chapters/07-heap-hash-avl.md:90-109` now gives LL/RR/LR/RL cases and concrete right-rotation rewiring plus refresh order.
   - Evidence: `chapters/07-heap-hash-avl.md:16-30` expands heap sink/capacity failure behavior, and `chapters/07-heap-hash-avl.md:50-60` expands rehash as collect/build/commit transaction.
   - Evidence: `exercises/L08_avl_tree/README.md:16-26` makes the inspector part of the exercise contract and repeats the core insertion/rotation rules at exercise level.

CMake wiring:

- `exercises/L08_avl_tree/CMakeLists.txt:12-30` registers `bad_vector` and `bad_stale_height` only inside `if(RANGES_BUILD_REFERENCE)`.
- Independent `build-review-r2-student` configured with `-DRANGES_BUILD_REFERENCE=OFF -DRANGES_TEST_STUDENTS=ON`; generated validation/bad project count was 0.
- Independent `build-review-r2-notests` configured with `-DBUILD_TESTING=OFF -DRANGES_BUILD_REFERENCE=ON`; configure succeeded and `ctest -N` reported `Total Tests: 0`.

Validation run:

- L08 `build-review-r2`: configure passed; Debug build passed; Debug CTest 5/5 passed.
- L08 `build-review-r2`: Release build passed; Release CTest 5/5 passed.
- L08 `build-review-r2-student`: configure/build passed; student CTest exited 8 with `check failed: new value inserts`, as expected.
- Static scan of the r2 L08 slice found no credential-like strings, broad catch, empty catch, TODO/FIXME, or fallback masking path.
- C++ LSP diagnostics tool was not available in this lane; MSVC configure/build covered compile diagnostics for affected targets.

Non-blocking hardening note:

- [LOW] `exercises/L08_avl_tree/checks/avl_checks.cpp:69` checks duplicate insertion rejection and `:70` checks size, but does not re-run `inorder()` or `check_structure()` after the duplicate call. A malicious duplicate path could corrupt the tree after the last assertion in each `require_tree` case while keeping size unchanged. Fix: after line 70, recheck `set.inorder() == expected` and `check_structure(set, expected, message)`.

Stop condition:

The two original blockers are fixed and independently reverified. No wider repository scan was performed.
