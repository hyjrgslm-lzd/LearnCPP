# C09 Capstone4 RPC author evidence

Date: 2026-09-11. Scope: `C09_Coroutines/exercises/Capstone4_rpc_framework/**`,
`C09_Coroutines/13-第三阶段结课-RPC框架.md`, and this author evidence folder.

## Original contract gaps

- `include/rpc/protocol.hpp` described JSON/newline framing and lacked `Request::idempotent`,
  cancel frames, frame helpers, `read_frame`, and the 4096-byte bound required by the current spec.
- `src/client.cpp` and `src/server.cpp` kept `RpcClient`/`RpcServer`-shaped types private in `.cpp`
  files and returned placeholder behavior, so an external checker could not link or exercise student
  code through public headers.
- `src/main.cpp` was a permanent compile-only sentinel returning `2`; completing student code could
  not make the same target naturally pass.
- `CMakeLists.txt` registered only `Capstone4_rpc_framework_reference`; there was no student-linked
  checker, independent good variant, or representative bad rejection.

## Implemented behavior

- Public protocol contract: 8-byte decimal length header, max body 4096, `Q|id|idempotent|method|args`,
  `C|id`, `R|id|result|status`; `SerializationError` represents protocol/framing failure.
- Public client/server declarations: `rpc/client.hpp` exposes `RpcClient::connect/call/shutdown/in_flight`;
  `rpc/server.hpp` exposes `RpcServer::start/stop/register_handler/in_flight`.
- Student `src/` is a compileable TODO starting point. The semantic checker calls real student operations;
  default TODO throws are caught by the checker and produce bounded non-zero failure. `main` also reports
  the first TODO and returns non-zero instead of using a completion flag.
- The complete author implementation was moved to `good/` as a reference-adapted answer. It mirrors
  the current Reference structure and is not claimed as a clean-room independent implementation.
- `good_protocol_alt.cpp` is a separate protocol Part implementation. It is checked by the same semantic
  checker together with the reference-adapted answer client/server; this proves only the protocol Part is
  independently implementable.
- The checker covers normal calls, pending response routing, timeout, cancel frame, idempotent retry only,
  server error, unknown method, connection lost, late response discard, duplicate shutdown, and drain to
  `in_flight()==0`.
- CMake now builds `Capstone4_rpc_framework_student_lib`, `Capstone4_rpc_framework_student_check`,
  `Capstone4_rpc_framework_answer_check`, `Capstone4_rpc_framework_protocol_good_check`,
  `Capstone4_rpc_framework_bad_newline_rejected`, and `Capstone4_rpc_framework_reference`.
- `COROUTINE_STUDY_TEST_STARTERS=OFF` does not register student or answer checks; Reference remains
  controlled only by `COROUTINE_STUDY_BUILD_REFERENCE`.

## Verification

Configure:

```powershell
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build/c09-author-rpc-asio -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DASIO_INCLUDE_DIR=F:/CPPTrain/LearnCPP/C09_Coroutines/exercises/build/full-windows/_deps/asio-src/include -DCOROUTINE_STUDY_FETCH_DEPS=OFF -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_TEST_STARTERS=ON
```

Result: configure/generate completed; build files written to
`F:/CPPTrain/LearnCPP/C09_Coroutines/exercises/build/c09-author-rpc-asio`.

Build:

```powershell
cmake --build C09_Coroutines/exercises/build/c09-author-rpc-asio --config Release --target Capstone4_rpc_framework Capstone4_rpc_framework_student_check Capstone4_rpc_framework_answer_check Capstone4_rpc_framework_protocol_good_check Capstone4_rpc_framework_bad_newline_check Capstone4_rpc_framework_reference -- /m:1 /nr:false /p:UseMultiToolTask=false /p:CL_MPCount=1
```

Result: all six requested targets built successfully.

Student CTest, expected to fail:

```powershell
ctest --test-dir C09_Coroutines/exercises/build/c09-author-rpc-asio -C Release -R "Capstone4_rpc_framework_student_check" --output-on-failure
```

Result: exit non-zero, 0/1 passed, `Capstone4_rpc_framework_student_check` failed with
`check failed: uncaught exception from implementation`. No `WILL_FAIL` is set for Student.

Answer/reference/protocol-good/bad supervised CTest:

```powershell
ctest --test-dir C09_Coroutines/exercises/build/c09-author-rpc-asio -C Release -R "Capstone4_rpc_framework_(answer_check|protocol_good_check|reference|bad_newline_rejected)$" --output-on-failure
```

Result: 4/4 passed.

- `Capstone4_rpc_framework_answer_check`: passed, 0.72s.
- `Capstone4_rpc_framework_protocol_good_check`: passed, 0.71s.
- `Capstone4_rpc_framework_bad_newline_rejected`: passed through `run_test.py`, 0.52s.
- `Capstone4_rpc_framework_reference`: passed, 0.25s.

Bad raw supervision:

```powershell
python C07_OS_Memory_System_IO/exercises/tools/run_test.py --name Capstone4_rpc_framework_bad_newline_rejected_manual --records C09_Coroutines/references/validation/c09-refresh/authors/rpc/records --timeout 60 --expect-exit 1 --contains "check failed: frame header uses 8-byte decimal length" -- C09_Coroutines\exercises\build\c09-author-rpc-asio\Capstone4_rpc_framework\Release\Capstone4_rpc_framework_bad_newline_check.exe
```

Result: exit 0 from supervisor, child exit matched 1, required text matched:
`check failed: frame header uses 8-byte decimal length`.

Reference-only registration with `COROUTINE_STUDY_TEST_STARTERS=OFF`:

```powershell
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build/c09-author-rpc-refonly -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DASIO_INCLUDE_DIR=F:/CPPTrain/LearnCPP/C09_Coroutines/exercises/build/full-windows/_deps/asio-src/include -DCOROUTINE_STUDY_FETCH_DEPS=OFF -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_TEST_STARTERS=OFF
ctest --test-dir C09_Coroutines/exercises/build/c09-author-rpc-refonly -C Release -N -R "Capstone4_rpc_framework"
```

Result: configure passed; `ctest -N` listed only `Capstone4_rpc_framework_reference`.

Demo before restoring Student TODO status:

```powershell
C09_Coroutines\exercises\build\c09-author-rpc-asio\Capstone4_rpc_framework\Release\Capstone4_rpc_framework.exe
```

Result: exit 0, `Capstone4_rpc_framework: RPC smoke passed`.

After restoring Student TODO status, the same demo now fails bounded by design:

```powershell
C09_Coroutines\exercises\build\c09-author-rpc-asio\Capstone4_rpc_framework\Release\Capstone4_rpc_framework.exe
```

Result: non-zero, `Capstone4_rpc_framework: TODO: implement rpc::RpcServer::start`.

## Source hashes

```text
A5FB1F76727F94C0B8AE80B8ED5BB2D42F41766DD8AAC8FDF80E84EBBF2B1A3A  include/rpc/protocol.hpp
DB8EF864B006E7FD224CC3F6819DC362D06BC73A62D787BFB48ACA812FD7B410  include/rpc/client.hpp
6F6D66AACF87B2F79275471B7D506ED382C4679F42B8E48C6898135A9BF99539  include/rpc/server.hpp
869440F94F7BFC6DE028FC2012F15DECCAE76E0321E30EBC82B1806FE15F417E  src/protocol.cpp
3084BF2ADF2B54ECB332076013F53460BF557D41F0D53C32CEE6532D4110F566  src/client.cpp
4CABFE4B117B45A47B85BC3E8E2A0D9649136AA60C805C3C52B1B7CF69CEBA32  src/server.cpp
6804C624140EC3A215B1CD1804F597CD9561D0448DE33A3066A5AC6A80572FF6  src/main.cpp
0A07CDAB53B2DBCCA9E5CF655018545132B691CB439543FE332DE9E054098289  good/protocol.cpp
376C423839CD36A942C712532FF3B4AEFA8B1D2DED1A06E40701E42E8E9D9D00  good/client.cpp
7837034E54E2E02F4217F985A557ED1ECAD96D35440CE96D34DCAC50D9312B82  good/server.cpp
3775380390A0F2E0F4D4C9F9581ED43AE2E71FEC6DC563520074C6712C5EEB7A  tests/rpc_student_test.cpp
845FE1D538B69BC5965378B7CD34915E1E959DA1AE8527DF74DE2E6311CA8957  authors/rpc/good_protocol_alt.cpp
BF9D71D408323B9A96A81CB254651573D89BDF4B83773B62584B89E45A350A6A  authors/rpc/bad_newline_protocol.cpp
C46C9FE1FD9FFFA01E7EF97697D89D7F0D5400D7844535D93D8C91491BF3B264  Capstone4_rpc_framework/CMakeLists.txt
19AB0DC6DB31077303A9D33360A8E57A2C234A84093D501209DF9A0002652452  Capstone4_rpc_framework/README.md
407B5903080E07B01905AEB4724141D02CD0EC8265AEAC75878B8803175ADA6F  13-第三阶段结课-RPC框架.md
```

## Not verified

- No WSL/Linux run in this slice.
- No ASan/TSan run in this slice.
- Non-author review still needs to verify that a learner can complete `src/` without opening Reference.
- `good/` is a reference-adapted answer retained for checker proof, not an independent
  clean-room solution.
