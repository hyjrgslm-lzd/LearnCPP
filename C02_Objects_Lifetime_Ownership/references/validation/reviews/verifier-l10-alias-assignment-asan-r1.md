# C02 verifier report — L10 rc_ptr alias assignment ASan repro r1

## Verdict

REQUEST_CHANGES.

The suspected legal alias-assignment bug is real in the frozen L10 reference implementation. Both cases are valid user code for an owning smart pointer shape:

```cpp
struct Node {
    explicit Node(int v) : value(v) {}
    int value;
    l10::rc_ptr<Node> child;
};

auto p = l10::make_rc<Node>(1);
p->child = l10::make_rc<Node>(2);
p = p->child;              // copy case
p = std::move(p->child);   // move case, separate process
```

In both cases, `other` is a subobject inside the object currently owned by `*this`. `operator=` calls `release()` before reading from `other`; releasing `p` destroys the parent `Node`, which destroys `child`, so the later read of `other.block_` is use-after-free.

## Source binding

Evidence directory: `Core_Study/references/validation/reviews/c02-l10-alias-assign-asan-r1/`.

`source-hashes.json` records the frozen source snapshot:

- `Core_Study/exercises/L10_control_block/src/reference/rc.hpp`: `8B472959A9AD0EDF4F33B8DBD6532B4CDFA50A07BE27BC37ECA6A4ECFB36207C`
- `Core_Study/exercises/L10_control_block/checks/support/rc_support.hpp`: `B5D3973ED2D984586475D5AF1F220F9753FF43D6A8AEDBDF0995C07960548826`
- `Core_Study/exercises/L10_control_block/checks/rc_checks.hpp`: `D79869745DAEA612D94EA9B0339AA54AA91A730D8DCE81472F83905FB3FF3FA4`

## Evidence

- `build-alias-asan-md.json` — PASS. Built `alias-copy-uaf.exe` and `alias-move-uaf.exe` with `clang-cl` 22.1.3 and `/fsanitize=address`. Build wrapper sets LLVM bin and ASan runtime dirs on PATH.
- `run-copy-asan.json` — PASS wrapper, process exit 1 as expected. ASan reports `heap-use-after-free`; top read is `l10::rc_ptr<Node>::operator=(rc_ptr<Node> const&)` at `Core_Study/exercises/L10_control_block/src/reference/rc.hpp:76`. The freed stack shows `release()` at `rc.hpp:133` called from copy assignment `rc.hpp:75`.
- `run-move-asan.json` — PASS wrapper, process exit 1 as expected. ASan reports `heap-use-after-free`; read occurs through `std::exchange(other.block_, nullptr)` called by move assignment at `rc.hpp:87`. The freed stack shows `release()` at `rc.hpp:133` called from move assignment `rc.hpp:86`.
- `l10-reference-assignment-lines.txt` — source inspection confirms copy assignment does `release(); block_ = other.block_; add_strong();`, and move assignment does `release(); block_ = std::exchange(other.block_, nullptr);`.

Preserved non-conclusion records:

- `build-alias-asan.json` — command-path typo; not a code result.
- `build-alias-asan-corrected.json` — Clang ASan rejected `/MDd`; fixed in `build-alias-asan-md.json` with `/MD`.

## Minimal fix suggestion

- Copy assignment must acquire the RHS control block before releasing the current one. Minimal safe shape:

```cpp
auto* incoming = other.block_;
if (incoming != nullptr) {
    ++incoming->strong;
}
release();
block_ = incoming;
```

or copy-and-swap with a temporary `rc_ptr tmp(other)`.

- Move assignment must detach the RHS before releasing the current one:

```cpp
auto* incoming = std::exchange(other.block_, nullptr);
release();
block_ = incoming;
```

Keep the existing `this != &other` self-move guard.

- Add this exact alias-member assignment case to L10 public/reference checks so the original matrix cannot miss it again.

## Secondary code review note

`rc_ptr(control_block_base*, bool)` is public in the reference implementation. That looks like an internal adopt constructor exposed to avoid earlier friend-template issues. It is not the ASan root cause above, but it leaks control-block internals into the exercise API. Prefer making adoption private/internal and granting only `make_rc` / `weak_rc::lock` the needed access.

## Gaps

- I did not patch or rerun a fixed implementation. This report proves the frozen source fails under ASan and gives the minimal repair direction.
- The repro is Clang ASan / Windows only, matching the requested toolchain. It is enough for this memory-safety claim because ASan produced concrete heap-use-after-free stacks at the target source lines.

## Risks

- The current L10 checker matrix did not include assignment from a subobject owned by the LHS. Without a new public check, the same regression can pass the existing good/bad matrix again.
