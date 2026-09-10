# Foundation Associative Review

Verdict: BLOCK

Scope reviewed:

- `chapters/05-associative-containers.md`
- `chapters/06-algorithm-contracts.md`
- `chapters/07-heap-hash-avl.md`
- `exercises/L03_associative_containers`
- `exercises/L04_algorithms`
- `exercises/L06_binary_heap`
- `exercises/L07_chained_hash`
- `exercises/L08_avl_tree`

Source baseline:

- HEAD: `f261bea559d6722c31135fb2d3589be52fe958ed`
- Source hashes and local review matrix: `C06_Ranges/validation/associative-review-build-matrix.json`

Stage 1 - spec compliance:

- `05-associative-containers.md` covers ordered equivalence, `multimap::equal_range`, node handles with allocator boundary, transparent lookup, unordered hash/equal contract, collisions, and rehash semantics.
- `06-algorithm-contracts.md` covers algorithm preconditions, partition/stable partition, strict weak ordering, binary search with matching comparator/projection, merge/set algorithms, heap semantics, erase-remove, fold, and projection.
- `07-heap-hash-avl.md` introduces the three implementation exercises, but AVL implementation teaching is not deep enough for the "hide Reference and implement from text" bar.
- L03/L04 observation exercises match the chapter claims and passed independent Debug/Release runs.
- L06/L07/L08 build and run, and the configured bad controls are rejected, but L08 checker does not prove the AVL exercise contract.

Stage 2 - code/security/quality:

- No credential-like strings found in the reviewed slice.
- No broad catch/silent exception pattern found.
- C++ LSP diagnostics tool was not available in this lane; MSVC configure/build covered compile diagnostics for all exercise targets in the reviewed slice.

Issues:

## [HIGH] AVL checker accepts a non-AVL fake implementation

File: `C06_Ranges/exercises/L08_avl_tree/validation/good/avl_set.hpp:9`

`validation/good/avl_set.hpp` implements `AvlSet` as a sorted `std::vector`, and `is_balanced()` returns `true` unconditionally at line 31. The same checker passes this target in Debug and Release. That proves the current checker validates the value-level public behavior, but not the structural AVL requirement from `chapters/07-heap-hash-avl.md:48` and `L08_avl_tree/README.md:14`.

Risk: a student can satisfy the tests without implementing an AVL tree, height maintenance, or rotations. The current bad control only rejects the "single rotations only" implementation. It does not reject the sorted-vector fake, stale height metadata, or `is_balanced()` self-reporting.

Fix: split value oracle and structural oracle. Keep the value checks for `insert/contains/inorder/size`, but add a checker-visible structural contract that does not trust `is_balanced()` alone. Minimal shape: expose a testing-only node snapshot/level-order traversal or equivalent inspector for L08, then recompute BST order, stored height, and balance factor from the reported structure. Add at least two more bad controls: sorted-vector fake and stale-height/misreported-balance fake. Keep the existing no-LR/RL bad control.

## [HIGH] AVL teaching text does not support implementing the hidden Reference

File: `C06_Ranges/chapters/07-heap-hash-avl.md:37`

The chapter states the AVL invariant and names LL/RR/LR/RL rotations, but it does not show the pointer/subtree rewiring, the order of height refreshes, or how insertion returns the new subtree root during unwind. `L08_avl_tree/README.md:16` gives only one summary sentence. This misses the implementation-spec requirement that正文 explain background, preconditions, cause/effect, tradeoffs, and runnable implementation knowledge rather than leaving the implementation to Reference.

Risk: with Reference hidden, the learner gets terms and API requirements but not enough mechanism to implement the exercise reliably. Passing tests then becomes a search against checker behavior instead of applying the lesson.

Fix: expand `chapters/07-heap-hash-avl.md` or `L08_avl_tree/README.md` with compact pseudocode for recursive insert, `height(nullptr)=0`, `refresh`, balance factor, `rotate_left`, `rotate_right`, LR/RL double-rotation order, ownership/root-return lifecycle, duplicate insert behavior, and height validation. One page is enough; do not move the explanation into code comments only.

Validation run:

- `cmake -S ... -B build-review -DRANGES_BUILD_REFERENCE=ON`
- `cmake --build ... --config Debug`
- `ctest --test-dir ... -C Debug --output-on-failure`
- `cmake --build ... --config Release`
- `ctest --test-dir ... -C Release --output-on-failure`
- For L06/L07/L08, also `build-review-student` with `RANGES_TEST_STUDENTS=ON`; student CTest exited 8 as expected.

Observed results:

- L03: Debug 1/1 passed, Release 1/1 passed.
- L04: Debug 1/1 passed, Release 1/1 passed.
- L06: Debug 3/3 passed, Release 3/3 passed, Student failed as expected.
- L07: Debug 3/3 passed, Release 3/3 passed, Student failed as expected.
- L08: Debug 3/3 passed, Release 3/3 passed, Student failed as expected.

Stop condition:

The review is blocked on the two L08 issues above. No wider repository scan was performed.
