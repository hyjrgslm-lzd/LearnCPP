# C08 async logging review 2026-09-10

Review role: non-author logging sample and topic review.

Reviewed scope:

- `C08_Concurrency/exercises/cmake/SpdlogSetup.cmake`
  - SHA256: `B9D5C8B2F2C833ED2364FDAAAAF76290374A1A2D75704A0B1CBA83C35061937D`
- `C08_Concurrency/exercises/U01_async_logging/README.md`
  - SHA256: `BD4E5636E3A0C0733C0A15314F200E26AE26780DBC4B8E7662720A3EE6159F00`
- `C08_Concurrency/exercises/U01_async_logging/CMakeLists.txt`
  - SHA256: `FEDCE43C51C3E334F8B3F4139524D6E4C32DF77C85249366DF0AE5E3EF35E33A`
- `C08_Concurrency/exercises/U01_async_logging/check_main.cpp`
  - SHA256: `8AB231B77447DD8345B651C5875AEE8775BD11DE3104A70FFE86195306BA9612`
- `C08_Concurrency/exercises/U01_async_logging/checks.hpp`
  - SHA256: `105BF3C75AED3B1CB8D45AD2D3106F0A712E1D1D50DA31CA42432AEC2C2030C8`
- `C08_Concurrency/exercises/U01_async_logging/main.cpp`
  - SHA256: `09652231F033AB85C69A8024C8BD6B7AFA1BBD150C82A8453D0B758BDF68CA1B`
- `C08_Concurrency/exercises/U01_async_logging/solution.cpp`
  - SHA256: `F456F8FB62B35F4DBD1F9CFF4C48952F092FF89B85471CC0AD2B6CC2F295EB14`
- `C08_Concurrency/exercises/U01_async_logging/expect_failure.cmake`
  - SHA256: `39E4769E9C801F19643ABF99DE0063FC0B3D0EECB91919CF9F10A10071E969C4`
- `C08_Concurrency/exercises/U01_async_logging/src/student/async_logging_submission.hpp`
  - SHA256: `313DEC8442F076CEAE325124B7BFE12EF76A0741EBFF79E9B826B6CA3D1841D2`
- `C08_Concurrency/exercises/U01_async_logging/src/reference/async_logging_submission.hpp`
  - SHA256: `27A464D6A1E2FA68B8CD69E48898E877BC91BFA4ACEE2274EC4E178A774F733F`
- `C08_Concurrency/exercises/U01_async_logging/validation/good/async_logging_submission.hpp`
  - SHA256: `F655FBBA0431AC61EAC346E107CFDC999ED94144DEDC628C2B15631E452DB961`
- `C08_Concurrency/exercises/U01_async_logging/validation/bad_overflow/async_logging_submission.hpp`
  - SHA256: `288B686F683AC011D525C727A1C9B38EDB9F3E60E5D086E036649780AF8A7A20`
- `C08_Concurrency/exercises/U01_async_logging/validation/bad_flush/async_logging_submission.hpp`
  - SHA256: `226F1A214F7BE2BE323BC7BD8642699F361EA8BBC15575269CE7531756A1AFB5`
- `C08_Concurrency/topics/logging/01-async-spdlog.md`
  - SHA256: `0B6DE0A175F1CEA1734134B0FC6CF8133CD67C2609D7A0AD8D1B902BC552EB7A`
- `C08_Concurrency/topics/logging/02-spdlog-source-reading.md`
  - SHA256: `B8B71B338EE666EBE3585BC6194144912862F5B6AA711A663A0C4994C50AA8AE`
- `C05_Data_Representation_Standard_Facilities/chapters/20-spdlog-frontend.md`
  - SHA256: `307CBAA2BE32EC5EC1AD917B5A6B3E6CDF0FACD95D6BD5E8A2AF6D8A338FBB29`
- `C08_Concurrency/exercises/U01_async_logging/validation/evidence-20260910-async-logging.json`
  - SHA256: `26DC97397CAACCF079C637E0A440878B80FEFCC5DEC0D1C4ACFE3C7ADDE9A4CA`

## Verdict

REVISE.

The U01 teaching content, checker shape, good/bad validation design, source pinning, default-off dependency boundary, and Release/ASan runtime behavior are acceptable. One infrastructure detail still blocks approval: U01 custom validation tests hard-code shorter timeouts and do not fully inherit the sanitizer timeout policy.

## Blocking finding

1. P1 - U01 custom validation tests still override sanitizer timeout with fixed 30/20 second limits.

   Evidence: `U01_async_logging/CMakeLists.txt:44` through `U01_async_logging/CMakeLists.txt:46` set `U01_async_logging_good`, `U01_async_logging_bad_overflow_rejected`, and `U01_async_logging_bad_flush_rejected` to `TIMEOUT 30`. In the generated ASan test file, `build/c08-logging-asan/U01_async_logging/CTestTestfile.cmake:9` shows the `U01_async_logging_reference` test correctly has `TIMEOUT "120"` from `cs_add_exercise`, but `CTestTestfile.cmake:15`, `CTestTestfile.cmake:21`, and `CTestTestfile.cmake:27` show the independent good/bad validation tests still have `TIMEOUT "30"`. `expect_failure.cmake:2` also wraps each bad executable with an inner `TIMEOUT 20`.

   Risk: the ASan build currently passes because the U01 checks are fast, but the configured ASan validation window is not actually 120 seconds for three of the four U01 tests. This recreates the old "preset says 120, per-test property wins with 30" class of issue for U01-specific validation targets. The inner `expect_failure.cmake` timeout is another effective cap for the negative cases.

   Minimal fix: set the three U01 custom CTest properties to `TIMEOUT ${CS_TEST_TIMEOUT}`. For bad variants, pass the same timeout into `expect_failure.cmake` or otherwise document that the inner 20-second process bound is intentionally stricter and independent of sanitizer timeout. After that, regenerate the ASan build tree and confirm the generated U01 good/bad test entries no longer show `TIMEOUT "30"` under sanitizer.

## Passing checks

1. Student path is a real submission surface and is answer-shielded.

   `main.cpp:8` rejects the initial starter when `complete=false`, and `src/student/async_logging_submission.hpp` exposes the five required operations with TODO failures. `README.md:15` through `README.md:23` names the exact implementation surface and parts without embedding the reference body. A student can implement `make_pool`, `make_logger`, `log`, `request_flush`, and `shutdown` from the mechanism described in the README and logging topics.

2. Reference and independent good validation are separate.

   `solution.cpp` uses `src/reference/async_logging_submission.hpp`. `U01_async_logging/CMakeLists.txt:21` through `U01_async_logging/CMakeLists.txt:28` builds independent validation targets from `validation/good`, `validation/bad_overflow`, and `validation/bad_flush` through `check_main.cpp`; the good header has its own logger name and calls `logger->log(...)` rather than reusing the reference file.

3. Bad variants test real mistakes through the same checker.

   `validation/bad_overflow/async_logging_submission.hpp:20` through `validation/bad_overflow/async_logging_submission.hpp:21` intentionally maps `overrun_oldest` to `discard_new`, and the shared checker rejects it at `checks.hpp:164` with `overrun counter`. `validation/bad_flush/async_logging_submission.hpp:30` drops `request_flush`, and the shared checker rejects it at `checks.hpp:278` with `queued flush runs during drain`. `expect_failure.cmake:4` through `expect_failure.cmake:8` requires exit code `1` and the fixed `check failed: ...` diagnostic; it does not use `WILL_FAIL` or a separate expected-output implementation.

4. Gate cleanup and async failure scenarios are defensible.

   `checks.hpp:56` through `checks.hpp:68` define `gate_guard`, whose destructor opens the gate unless already released. Blocking tests create the guard before waiting on the worker and release it before waiting on the async future or shutdown. The block-policy path at `checks.hpp:187` through `checks.hpp:195` releases the gate before `third.get()`. Overflow, multi-worker, payload, and flush paths use the same guard shape, so assertion failures unwind through the guard instead of leaving the worker permanently stuck.

5. Payload ownership and execution threads are checked.

   The formatter writes the formatting thread at `checks.hpp:33` through `checks.hpp:38`. `checks.hpp:240` submits the original string, mutates it at `checks.hpp:241`, then verifies formatting happened on the caller thread, sink ran on the worker thread, and the sink observed `"owned"` rather than `"mutated"` at `checks.hpp:242` through `checks.hpp:248`.

6. Error handling, flush, and pool lifetime contracts are covered.

   `checks.hpp:255` through `checks.hpp:263` verifies sink exceptions reach `error_handler`. `checks.hpp:271` through `checks.hpp:278` verifies `flush()` is a queued request and runs during drain. `checks.hpp:286` through `checks.hpp:294` verifies `discard_new` may drop a flush request. `checks.hpp:298` through `checks.hpp:305` verifies expired `thread_pool` errors for both log and flush.

7. Source pinning and default-off dependency boundary are acceptable.

   `SpdlogSetup.cmake:3` defaults `CONCURRENCY_STUDY_ENABLE_SPDLOG` to `OFF`. `SpdlogSetup.cmake:11` through `SpdlogSetup.cmake:33` checks fixed git HEAD and tracked dirty state for fmt/spdlog, and `SpdlogSetup.cmake:69` through `SpdlogSetup.cmake:72` pins fmt `407c905e45ad75fc29bf0f9bb7c5c2fd3475976f` plus spdlog `79524ddd08a4ec981b7fea76afd08ee05f83755d`. `SpdlogSetup.cmake:82` through `SpdlogSetup.cmake:99` builds static fmt/spdlog with install/tests/examples/bench disabled. Reviewer `git status --short` for both dependency source checkouts was empty.

8. Topics and C05 bridge have the right ownership boundary.

   `topics/logging/01-async-spdlog.md:3` assigns formatter/logger/sink frontend to C05 and async queue/worker/backpressure/shutdown to C08. `topics/logging/02-spdlog-source-reading.md:3` fixes the source versions and `02-spdlog-source-reading.md:5` through `02-spdlog-source-reading.md:15` gives a concrete source-reading path. `C05_Data_Representation_Standard_Facilities/chapters/20-spdlog-frontend.md:3` states C05 only covers the synchronous frontend, and `20-spdlog-frontend.md:5` links forward to C08 U01 for the async runtime contract. This is a link-only C05 change in the reviewed line range.

## Fresh reviewer evidence

Commands run from `F:\CPPTrain\LearnCPP\C08_Concurrency\exercises`:

```text
cmake --build build/c08-logging-author --config Release --target U01_async_logging_checked
ctest --test-dir build/c08-logging-author -C Release -R U01_async_logging --output-on-failure
```

Result: PASS. Release U01 tests: `4/4` passed; total real time `2.46 sec`.

```text
cmake --build build/c08-logging-asan --config RelWithDebInfo --target U01_async_logging_checked
ctest --test-dir build/c08-logging-asan -C RelWithDebInfo -R U01_async_logging --output-on-failure
```

Result: PASS. ASan U01 tests: `4/4` passed; total real time `0.74 sec`.

Direct failure checks:

```text
build/c08-logging-author/U01_async_logging/Release/U01_async_logging_bad_overflow.exe
build/c08-logging-author/U01_async_logging/Release/U01_async_logging_bad_flush.exe
build/c08-logging-author/U01_async_logging/Release/U01_async_logging.exe
```

Observed outputs:

- bad overflow: `check failed: overrun counter`, exit `1`
- bad flush: `check failed: queued flush runs during drain`, exit `1`
- starter: `STARTER INCOMPLETE: U01 Part 1-4 未完成；未启动 async logger。`, exit `1`

Evidence artifact `validation/evidence-20260910-async-logging.json` already records Release configure/build/ctest, default-off configure, ASan configure/build/ctest, starter rejection, and materials registration, all with expected pass/reject status.

Stop condition: review report written only here; no reviewed content was edited.
