# Foundations teaching review r1

Verdict: ITERATE

Reviewer: non-author teaching review
Scope: chapters 00/01/02/07, exercises L01_invariants/L02_value_semantics/L07_interfaces, author-a-l01-l02 and author-a-l07 evidence.
Git HEAD observed: `8e0f407afd155ede81f0b06a51553dadef206a23`
Workspace note: `C03_Type_Modeling_Interface_Design/` is untracked at review time, so this review binds file-content hashes below rather than a committed tree.

## Source snapshot

Chapters and READMEs:

- `chapters/00-route-and-boundaries.md`: `9215D6FC5536C0BDE7119C639745F4C7BC709FDF3E4A3538B6D60DC5FD2C529B`
- `chapters/01-class-invariants.md`: `0278051206907970FAB1D4A15CFBAF21065FF64E7394ACED85293E3ACD95711C`
- `chapters/02-value-semantics.md`: `60477869A8E45C35516714E38D11429B9B202A97282DAD65ED868FC1DCCD4174`
- `chapters/07-interfaces-and-polymorphism.md`: `EB5E20D838CD8C963C19D3A399DF451BD8D39230C675FD37C66198797F9EBB98`
- `exercises/L01_invariants/README.md`: `9856F2CC1E36CE4B5EF1FFFB4CB7A894344AE103845AC00006B3AF72A8A659FA`
- `exercises/L02_value_semantics/README.md`: `6F0922B4489E7766570B0CE4E9274BF853B2EFCDB0A77E39316E9D268258333F`
- `exercises/L07_interfaces/README.md`: `9BEFC04131F6531C0D767D5BD704C1F115918E22C67ED23AACAC89557FCA6EAF`

Exercise build and validation files:

- `exercises/L01_invariants/CMakeLists.txt`: `C2FE1EDB1EC3A25C3BF39690541CD9735E59BFA6FDE75FFBA269A551FE54F0BA`
- `exercises/L01_invariants/checks/invariant_checks.cpp`: `8B89E98246BAD38DE7B4EC3712EE3D17AE7A0B54660EC59CB54D2E8F6F565E0C`
- `exercises/L01_invariants/src/reference/bounded_int.hpp`: `F20B9D3715DE7D7C175A60EDA5E84C7C06CA5D96AE1A67315812813470672602`
- `exercises/L01_invariants/src/student/bounded_int.hpp`: `1F13AE564FF5397184162E3E0A601BF662ADB92A3E89AA1083E8DD8B0A2B0BD1`
- `exercises/L01_invariants/validation/good/bounded_int.hpp`: `24E5BB28E93C22024BAEBECA1A2899534DA7F548B729A0092A0D0AC639712D7C`
- `exercises/L01_invariants/validation/bad/bounded_int.hpp`: `1F13AE564FF5397184162E3E0A601BF662ADB92A3E89AA1083E8DD8B0A2B0BD1`
- `exercises/L02_value_semantics/CMakeLists.txt`: `EDACBF42FB4D227FCD6F0600AA720E8F6E4EE0C31B2A5E31106112EA6DE0C9D7`
- `exercises/L02_value_semantics/checks/value_semantics_checks.cpp`: `8F921E00C81EEA1DF36EE9FA67F0A2A6ED6939CD12A3D953A3A64720E7E51DA0`
- `exercises/L02_value_semantics/src/reference/money.hpp`: `9EAF5F63446C197CF7638C816ED49B441BA02C779DFA76447651F57B24DB2E9B`
- `exercises/L02_value_semantics/src/student/money.hpp`: `7BEBCEEED75784F62538889E26D48F7196D9F8BE1E59CF9A9AE89FDBD30BA8AB`
- `exercises/L02_value_semantics/validation/good/money.hpp`: `695A1938F35EE8178C6AB3BE7ED1901D003B352C5DB0873ED0AC3873C431779F`
- `exercises/L02_value_semantics/validation/bad/money.hpp`: `7BEBCEEED75784F62538889E26D48F7196D9F8BE1E59CF9A9AE89FDBD30BA8AB`
- `exercises/L07_interfaces/CMakeLists.txt`: `943511E44BF84F4BC1C328AAD0AC20D2602B45D0EC6180FD180190109D5D96A0`
- `exercises/L07_interfaces/checks/interface_checks.cpp`: `9A87644E24B0BD4D357536B8C4EEB35883C6C21BA13AE09132BEBFA3BB5EBA4C`
- `exercises/L07_interfaces/checks/rvalue_view_negative.cpp`: `BE471F5649A82B65FACE28CB8C4C941333F069888AE83CA125EEF3A175C807DC`
- `exercises/L07_interfaces/src/reference/owner.hpp`: `44A7F752FF2C2974D66B73C6752B5E1D46268F9D8F84E9EB4D50180E399B442F`
- `exercises/L07_interfaces/src/student/owner.hpp`: `AE46C7A0835220ECA12C0C6A7D5F3475C646A832BFE58E3CC302AEEC585520B9`
- `exercises/L07_interfaces/validation/good/owner.hpp`: `7DA9DAC20A86220193047948E7D03479E5D38FFD7744993B4AE119D8A9F698D9`
- `exercises/L07_interfaces/validation/bad/owner.hpp`: `6D70AA9844E1D654B3EA81C461CD6C1F62218DE9FD71D1102C32EE851F7C179B`

Author evidence:

- `references/validation/author-a-l01-l02/summary-r2.md`: `18BD215F493B7C24B78E9C7E3DB41C0475D9D87DEA652C0A84CEC161DB56F6D2`
- `references/validation/author-a-l01-l02/source-sha256-r2.txt`: `86ABDE8E2FE9A2D640612AD1EF5AF790FFA96D42D3FCAA79CF4C9B0C1B87E671`
- `references/validation/author-a-l07/summary.md`: `3431B4FDAAC6DB01A13C3D7AB48E03672E51BDA55CA705B24C2D397523BAB6A9`
- `references/validation/author-a-l07/source-sha256.txt`: `9A6452E758F3A6B4ED35E5667E371C1D57B8DAA9B3B5FA3E2611C33B7EB66D79`

## Independent validation run

Minimum rerun focused on new risks:

- L01 core Debug: configure/build/ctest PASS, 3/3 tests passed.
- L02 core Debug: configure/build/ctest PASS, 3/3 tests passed.
- L07 core Debug: configure/build/ctest PASS, 4/4 tests passed.
- L01 student-on Debug: expected failure observed. `L01_invariants_student` failed with `check failed: constructor rejects values outside range`; other 3 tests passed.
- L02 student-on Debug: expected failure observed. `L02_value_semantics_student` failed with `check failed: different currencies cannot be added`; other 3 tests passed.
- L07 student-on Debug: expected failure observed. `L07_interfaces_student` failed with `check failed: snapshot remains independent after owner mutation`; other 4 tests passed.
- Direct compile of `L07_interfaces_rvalue_view_rejected` fails with MSVC `C2280` at `checks/rvalue_view_negative.cpp(5,34)`, attempting to call deleted `l07::Owner::view() &&` from `src/reference/owner.hpp(16,26)`.

Build cache paths used:

- `build/c03-critic-foundations-l01-core`
- `build/c03-critic-foundations-l02-core`
- `build/c03-critic-foundations-l07-core`
- `build/c03-critic-foundations-l01-student`
- `build/c03-critic-foundations-l02-student`
- `build/c03-critic-foundations-l07-student`

## Teaching assessment

The prose and exercise statements for 00/01/02/07 are mostly ready for this foundation slice. They give mechanism-level explanation for public-entry invariants, strong construction/factory boundaries, regular-like semantic laws, equality/order/hash consistency, borrow versus snapshot, const/ref-qualified access, and `noexcept` as a promise about a specific expression rather than no-fail semantics.遮住 Reference 后，L01/L02/L07 的 README 和 chapters are enough for a reader to know which student file to edit and what behavior to satisfy.

L01 and L02 validation shape is acceptable for this slice. The current bad implementations are also the default student stubs, but their first observed failures match the intended diagnostics and do not fail early for unrelated causes. L02 good is independently readable and exercises same-currency addition plus different-currency rejection. L01 is a small invariant wrapper, so close similarity between good and reference is not itself a blocker as long as no Reference include/link dependency exists.

## Blocking issue

### L07 temporary-object rejection is only tested against Reference, not against the selected implementation

Evidence:

- `exercises/L07_interfaces/CMakeLists.txt:12-16` creates `L07_interfaces_rvalue_view_rejected` with include directory fixed to `src/reference`.
- `exercises/L07_interfaces/checks/rvalue_view_negative.cpp:5` performs the relevant trigger: `l07::Owner{1, 2}.view()`.
- `exercises/L07_interfaces/checks/interface_checks.cpp:21-35` checks `view()` on lvalues, `snapshot()`, `replace_all()`, and const lvalue borrowing, but does not compile-probe that the selected implementation rejects rvalue `view()`.
- `exercises/L07_interfaces/validation/bad/owner.hpp:14-15` already has the correct `view() const&` plus deleted `view() &&`; its only intended failure is `snapshot()`.

Impact:

The course requires L07 to teach and validate borrowed view lifetime through const/ref qualification and temporary-object rejection. The observed compile-negative failure reason is real for Reference, but the validation harness would not reject a student or validation-good implementation that accidentally exposes `view()` on temporaries. Therefore L07 is not yet closed under its own stated Part 4 contract.

Minimal repair:

1. Make the rvalue-view compile-negative target exercise the same implementation selection contract as the normal checker, or add a dedicated `validation/bad_rvalue` implementation and target that proves an implementation with an rvalue-usable `view()` is rejected.
2. Keep the diagnostic cause precise: expected failure must be the temporary `view()` call at `checks/rvalue_view_negative.cpp:5`, not a missing include, wrong target, or unrelated checker assertion.
3. Rerun the minimum L07 matrix after the repair: core Debug including rvalue-negative, student-on Debug expected failure, and the new bad-rvalue rejection path if added.

## Verdict

ITERATE for L07 validation wiring only. The chapters and L01/L02 foundation slice can proceed from this snapshot, but the foundation batch should not be marked approved until L07 proves temporary-object rejection for non-Reference implementations.
