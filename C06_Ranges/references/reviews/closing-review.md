# C06 closing review

Verdict: BLOCK

Scope: closing review for F1/G3 independent validation/good implementations and the latest G2 stateful callable semantic fix. This review did not re-open unrelated C06 items and did not inspect C04/C05 changes.

## Source hashes

- `C06_Ranges/exercises/F1_iterator_hierarchy/src/reference/iterator_hierarchy.hpp`: `455118B70F1D05D7C3EF8BC95ED46C3A9C13D2CC3C337146C06EDF2E9DA81ED9`
- `C06_Ranges/exercises/F1_iterator_hierarchy/validation/good/iterator_hierarchy.hpp`: `24BAC0155D28523F51848687BA632D35D26630BB81F29A7B3861907541CD7696`
- `C06_Ranges/exercises/F1_iterator_hierarchy/validation/bad/iterator_hierarchy.hpp`: `84174530DF2A7FCF551B3FCD95BA5AECECDF697EEB30D6F680F6558DCB7719CE`
- `C06_Ranges/exercises/F1_iterator_hierarchy/main.cpp`: `E369EFF93A4D7390589B3002D0553D112090D30ECC9A4E1CDE5955876419D937`
- `C06_Ranges/exercises/G2_my_transform_closure/main.cpp`: `EFF61A5DFFC1986CB88C34630B53618388480FADDA7967DE8385487C2D44A957`
- `C06_Ranges/exercises/G2_my_transform_closure/src/reference/my_transform_view.hpp`: `2BEFF01E36587DED7BBD698DD818D1075D6EA4DE5AD95DDACF8850AEEF33F513`
- `C06_Ranges/exercises/G2_my_transform_closure/validation/good/my_transform_view.hpp`: `49A75B09FA94F405CE995012FCFEC2643AB9ED73A0C37D0A9863F13DB8BCDDB6`
- `C06_Ranges/exercises/G2_my_transform_closure/validation/bad/my_transform_view.hpp`: `2948E0947623FCBF1D9D6DB9C77AF51310AB19BEC48D56A12B72726807024D4F`
- `C06_Ranges/exercises/G3_my_enumerate_borrowed/src/reference/my_enumerate_view.hpp`: `86362150623C4F4EDE33A616A262E814D47712D70318B471F7E9986DB85F26C8`
- `C06_Ranges/exercises/G3_my_enumerate_borrowed/validation/good/my_enumerate_view.hpp`: `FE7FC41CBCDA1D1BDA55581C0868C998F839AA80E2346BA0DEBC6E0A92304CDC`
- `C06_Ranges/exercises/G3_my_enumerate_borrowed/validation/bad/my_enumerate_view.hpp`: `97D38017B3F7D34667D88C1263A0053E6124B8130E6DD6BDF9885499B56E8358`
- `C06_Ranges/exercises/G3_my_enumerate_borrowed/main.cpp`: `F827EA974B56E023F1BB8E991E2CD0046EF8B184E4F1CBAA07C83455B53129A6`

## Findings

### HIGH: G3 validation/good mishandles move-only prvalue elements

File: `C06_Ranges/exercises/G3_my_enumerate_borrowed/validation/good/my_enumerate_view.hpp:44`

`operator*()` stores `auto item = *current_`, then constructs `reference` with `std::get<1>(item)`. For a base range whose reference is a move-only prvalue, such as `views::transform` returning `std::unique_ptr<int>`, `item` is a local tuple and `std::get<1>(item)` is an lvalue. Constructing `pair<index_type, std::unique_ptr<int>>` from that tries to copy the `unique_ptr`. That breaks the public template contract for accepted value categories and makes the good oracle incomplete.

Same pattern exists in `operator[]()` at `C06_Ranges/exercises/G3_my_enumerate_borrowed/validation/good/my_enumerate_view.hpp:121`.

Fix: move the stored tuple when forwarding the element out:

```cpp
auto item = *current_;
return {read_index(item), std::get<1>(std::move(item))};

auto item = current_[n];
return {read_index(item), std::get<1>(std::move(item))};
```

`iter_move()` already uses `std::get<1>(std::move(item))`, so keep that shape.

### LOW: G3 checker narrows `ptrdiff_t` into `int` during vector construction

File: `C06_Ranges/exercises/G3_my_enumerate_borrowed/main.cpp:41`

MSVC emits C4244 because the transform lambda returns `item.first + item.second`, where `item.first` is `ptrdiff_t`, and the result is inserted into `std::vector<int>`. This is not a semantic blocker, but it adds warning noise to the closing evidence.

Fix: either cast the index in the lambda or collect into a vector whose value type matches the expression.

## Passed checks

- F1 good is independent from reference: reference uses custom `ptr_iterator<Tag>` over `vector`; good uses standard containers `forward_list`, `list`, `deque`, and `vector` as independent iterator-category oracles.
- G2 latest stateful check is semantically valid: `Stateful::operator()` is pure `value + delta`; copy constructor only records external diagnostic count; the checker verifies view-owned state, no dereference-time callable copies, and repeat traversal stability.
- G2 bad remains targeted: it copies the callable inside `operator*()`, and `BAD_DIAGNOSTIC` expects `dereference does not copy the callable`.

## Validation

- Configure: `cmake -S C06_Ranges/exercises -B C06_Ranges/validation/closing-review-build -G "Visual Studio 18 2026" -DRANGES_BUILD_REFERENCE=ON -DRANGES_TEST_STUDENTS=OFF -DRANGES_ENABLE_FRONTIER=OFF` passed with MSVC 19.51.
- Debug focused build: F1/G2/G3 reference, validation_good, validation_bad, plus G2 student target passed compilation.
- Debug focused CTest: 9/9 passed, covering F1/G2/G3 reference, validation_good, and validation_bad_rejected.
- Release focused build: same target set passed compilation.
- Release focused CTest: 9/9 passed.
- G2 student executable: Debug and Release both exited 1 with `check failed: prvalue transform produces expected values`.
- Static scan over F1/G2/G3 found no hardcoded secrets, broad fallback masking, or swallowed exceptions. `lsp_diagnostics` and `ast_grep_search` tools were not exposed in this worker; compiler diagnostics plus `rg` pattern scan were used instead.

## Stop condition

Blocked only on G3 validation/good value-category forwarding. No broader C06, C04, or C05 review was performed in this closing pass.
