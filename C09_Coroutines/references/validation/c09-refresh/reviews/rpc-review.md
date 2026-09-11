# Capstone4 RPC 非作者检查

检查日期：2026-09-11。

结论：APPROVE。

本次只读检查生产课程文件与 RPC 检查接线，只写本报告和 `rpc-review-evidence/**`。未修改 `Capstone4_rpc_framework` 的 Student、good、Reference、正文或 CMake。Reference 未作为教学可做性的输入；只运行 `Capstone4_rpc_framework_reference.exe` 验证独立答案仍通过。

## 被审版本

```text
407B5903080E07B01905AEB4724141D02CD0EC8265AEAC75878B8803175ADA6F  C09_Coroutines\13-第三阶段结课-RPC框架.md
19AB0DC6DB31077303A9D33360A8E57A2C234A84093D501209DF9A0002652452  C09_Coroutines\exercises\Capstone4_rpc_framework\README.md
D29A958ADD5D68DB70F73E0DF8B1B9A4FCBD2F4C31E471531ED4AA7DCB769F4A  C09_Coroutines\exercises\Capstone4_rpc_framework\CMakeLists.txt
A5FB1F76727F94C0B8AE80B8ED5BB2D42F41766DD8AAC8FDF80E84EBBF2B1A3A  C09_Coroutines\exercises\Capstone4_rpc_framework\include\rpc\protocol.hpp
DB8EF864B006E7FD224CC3F6819DC362D06BC73A62D787BFB48ACA812FD7B410  C09_Coroutines\exercises\Capstone4_rpc_framework\include\rpc\client.hpp
6F6D66AACF87B2F79275471B7D506ED382C4679F42B8E48C6898135A9BF99539  C09_Coroutines\exercises\Capstone4_rpc_framework\include\rpc\server.hpp
3775380390A0F2E0F4D4C9F9581ED43AE2E71FEC6DC563520074C6712C5EEB7A  C09_Coroutines\exercises\Capstone4_rpc_framework\tests\rpc_student_test.cpp
845FE1D538B69BC5965378B7CD34915E1E959DA1AE8527DF74DE2E6311CA8957  C09_Coroutines\references\validation\c09-refresh\authors\rpc\good_protocol_alt.cpp
BF9D71D408323B9A96A81CB254651573D89BDF4B83773B62584B89E45A350A6A  C09_Coroutines\references\validation\c09-refresh\authors\rpc\bad_newline_protocol.cpp
```

## 只读审查结论

- Public API 完整：`protocol.hpp` 暴露 `Request/Response/RpcError`、`max_frame_size=4096`、8 字节帧、`encode_cancel`、`read_frame`、`parse_request/parse_response/parse_cancel`；`Request::idempotent` 默认 `false`；`RpcClient::call(..., retries = 0)` 保留旧两参数调用。
- Student stub 安全：`src/protocol.cpp/client.cpp/server.cpp` 以 `TODO: implement ...` 抛出或执行有限 close/clear，不会无限线程、外部连接或假返回成功。
- 同一 semantic checker 覆盖真实路径：协议 roundtrip/坏字段/超长 body/坏 header、回环临时端口、6 个并发请求、timeout/cancel、幂等 retry、connection_lost fail_all、重复 shutdown、client/server `in_flight()==0`。
- `good/` 是 reference-adapted answer，只作为 answer 通过证据，不算独立 good。`good_protocol_alt.cpp` 只替换 protocol Part，client/server 仍走 answer，用来证明协议阶段可独立完成。
- `bad_newline_protocol.cpp` 是行为型坏协议；用 `run_test.py` 精确要求 exit 1 和 `check failed: frame header uses 8-byte decimal length`，不是任意失败算绿。
- `rg "rpc_ref|reference/include|Capstone4_rpc_framework/reference|#include.*reference|#include.*rpc_ref"` 对 Student checker、good、authors/rpc、public include、src 无命中，未发现直接 include Reference。

## CMake 接线

配置 A：`c09-review-rpc-refon`

```text
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build/c09-review-rpc-refon -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_TEST_STARTERS=OFF -DCOROUTINE_STUDY_FETCH_DEPS=OFF -DASIO_INCLUDE_DIR=F:/CPPTrain/LearnCPP/C09_Coroutines/exercises/build/capstone4-asio/_deps/asio-src/include
cmake --build C09_Coroutines/exercises/build/c09-review-rpc-refon --config Release --target Capstone4_rpc_framework_answer_check Capstone4_rpc_framework_protocol_good_check Capstone4_rpc_framework_bad_newline_check Capstone4_rpc_framework_reference
```

结果：配置成功，四个 Release exe 均生成。CTest 注册 answer/protocol_good/bad/reference；未注册 Student。

配置 B：`c09-review-rpc-student`

```text
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build/c09-review-rpc-student -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=OFF -DCOROUTINE_STUDY_TEST_STARTERS=ON -DCOROUTINE_STUDY_FETCH_DEPS=OFF -DASIO_INCLUDE_DIR=F:/CPPTrain/LearnCPP/C09_Coroutines/exercises/build/capstone4-asio/_deps/asio-src/include
cmake --build C09_Coroutines/exercises/build/c09-review-rpc-student --config Release --target Capstone4_rpc_framework_student_check
```

结果：配置成功，只生成 `Capstone4_rpc_framework_student_lib` 与 `Capstone4_rpc_framework_student_check` 相关 target；未生成 answer/protocol_good/bad/reference target。CTest 注册 Student；未发现 `WILL_FAIL` 属性。

## 运行证据

每个运行都由 `C07_OS_Memory_System_IO/exercises/tools/run_test.py` 监督，记录写入 `C09_Coroutines/references/validation/c09-refresh/reviews/rpc-review-evidence/records`。

```text
rpc_student_expected_fail-20260911T030323224000Z.json: verdict=PASS exit=1 timeout=False required='check failed: uncaught exception from implementation'
rpc_answer_pass-20260911T030322776375Z.json: verdict=PASS exit=0 timeout=False required=''
rpc_protocol_good_pass-20260911T030322729899Z.json: verdict=PASS exit=0 timeout=False required=''
rpc_reference_pass-20260911T030322732899Z.json: verdict=PASS exit=0 timeout=False required=''
rpc_bad_newline_rejected-20260911T030322514582Z.json: verdict=PASS exit=1 timeout=False required='check failed: frame header uses 8-byte decimal length'
```

判定：

- 最终真实 Student FAIL 成立，且不是 `WILL_FAIL` 伪绿。
- answer PASS 成立，但仅作为 reference-adapted answer 证据。
- protocol_good PASS 成立，证明协议 Part 有独立可做路径。
- bad newline 被精确消息拒绝，checker 有基本拒错能力。
- Reference PASS 成立，隔离答案仍可运行。

## 风险与边界

- 本次不做逐 Part 教学终审；只核对正文/README 足以让代表阶段不偷看 Reference 推进到 protocol、client/server、timeout/cancel/retry/drain。
- CTest 中 bad test 的 `--records` 仍指向作者 `authors/rpc/records`；本次为遵守审查写入边界，未运行该 CTest 条目，而是手动用同一 bad exe 和精确断言写入本审查 evidence。这个不阻断课程验收，但最终汇总报告应说明作者记录与审查记录来源不同。
- 本次不重复 mini/shared runtime 审查。
