# Capstone5 mini Student 可做性独立验证

验证日期：2026-09-11。

结论：APPROVE。

本次只验证新补 Student checker 的可做性与拒错能力，不批准生产 Student/Reference/正文最终质量。验证时遮住 `C09_Coroutines/exercises/Capstone5_mini_corolib/reference`，只读取 14 正文、项目 README、Student `include/mini` 接口和 `tests/`。候选实现只写入 `mini-student-evidence/**`，构建产物只写入 `C09_Coroutines/exercises/build/c09-review-mini/**`。

## Include 隔离

good candidate 编译使用 include 顺序：

```text
-I C09_Coroutines/references/validation/c09-refresh/reviews/mini-student-evidence/good/include
-I C09_Coroutines/exercises/Capstone5_mini_corolib/include
-I C09_Coroutines/exercises/Capstone5_mini_corolib/tests
-I C09_Coroutines/exercises/include
```

`g++ -MM` 回读依赖显示：

- `when_any_test.cpp` 使用 evidence `good/include/mini/when_any.hpp`、Student `mini/task.hpp`、`manual_event.hpp`、`exercise_check.hpp`。
- `shared_task_test.cpp` 使用 evidence `good/include/mini/shared_task.hpp`、Student `mini/task.hpp`、`manual_event.hpp`、`exercise_check.hpp`。
- `run_loop_test.cpp` 使用 evidence `good/include/mini/single_thread_executor.hpp`、Student `mini/task.hpp`、`exercise_check.hpp`。

三条依赖列表均未出现 `C09_Coroutines/exercises/Capstone5_mini_corolib/reference`。

## Source SHA256

```text
0EA418C4881BFBF48C72C9F3B29388AD9F176BAB77A36BBE6768A946C9577CA6  C09_Coroutines\14-第三阶段结课-mini协程库实现.md
19E712FAB0B97E20A156B5CABA00674BED5B09934BD5040C1048796D99CABEB1  C09_Coroutines\exercises\Capstone5_mini_corolib\README.md
2291BCF20939566AACF83F091A51ADD0CD33197A1140DA67E42A7A9DEFE9966A  C09_Coroutines\exercises\Capstone5_mini_corolib\tests\when_any_test.cpp
9B147F88D1456B1FD44B79FEF88662DE98205D3314F7F16C3E160B59FEC00C6D  C09_Coroutines\exercises\Capstone5_mini_corolib\tests\shared_task_test.cpp
F1017618716A1F041749195F32C4CD06DD229507CD0BDE290C9CDA24B1257E1A  C09_Coroutines\exercises\Capstone5_mini_corolib\tests\run_loop_test.cpp
3CF21CD8DABA47716BDBFF41E5E9CB7BA61FB7984D55AD66A1430DA9F5D378BA  C09_Coroutines\exercises\Capstone5_mini_corolib\tests\stop_test.cpp
361E5D7C0CCD122B4957F8AEF687EBD15F224DAFB257B29767CD4ECBC847F9B6  C09_Coroutines\exercises\Capstone5_mini_corolib\tests\manual_event.hpp
293BCA33CC79638309F60ECF9169C745B3BCCF504D329C010BE251A0665BC0B8  C09_Coroutines\exercises\Capstone5_mini_corolib\include\mini\task.hpp
DD7F58B4187E5F6F95F00FEE1D4DDCCF41EC7679D8BC9465EAF6DC493D286081  C09_Coroutines\exercises\Capstone5_mini_corolib\include\mini\stop_token.hpp
2725162FD88DBE48F4DA5441410B8D592F3CAD4C6E1DC1313490123DAE9D9D07  C09_Coroutines\references\validation\c09-refresh\reviews\mini-student-evidence\good\include\mini\when_any.hpp
17B7CB1FB5B9E85F12C00FFCFA0AD987AADDDFCB7501A5F79E4A47F30FEBFF99  C09_Coroutines\references\validation\c09-refresh\reviews\mini-student-evidence\good\include\mini\shared_task.hpp
B2618D3AB848ADA0F4089A58B845433D09D4D35E5B148AD7FB88E2F2BB9F9E53  C09_Coroutines\references\validation\c09-refresh\reviews\mini-student-evidence\good\include\mini\single_thread_executor.hpp
B82527737464E7556C617E91A148DA30116F89D814F5411897507DF4B2F503F5  C09_Coroutines\references\validation\c09-refresh\reviews\mini-student-evidence\bad_when_any_no_drain\include\mini\when_any.hpp
4E66BC93A2BC46CAD5C841CBDC2489078E8F157AB12BACB6E0B4E2E4F4CE6C06  C09_Coroutines\references\validation\c09-refresh\reviews\mini-student-evidence\bad_shared_single_waiter\include\mini\shared_task.hpp
8E74984679BCDD3F2C928E626332E1D7B58F215296D0F49D634C6B4D66790A8B  C09_Coroutines\references\validation\c09-refresh\reviews\mini-student-evidence\bad_executor_inline\include\mini\single_thread_executor.hpp
```

## 编译

环境：WSL Ubuntu GCC 13.3.0，`-std=c++20 -Wall -Wextra -pedantic`。

同一批 checker 未修改，直接编译：

```text
g++ ... tests/when_any_test.cpp      -o C09_Coroutines/exercises/build/c09-review-mini/good_when_any_test
g++ ... tests/shared_task_test.cpp   -o C09_Coroutines/exercises/build/c09-review-mini/good_shared_task_test
g++ ... tests/run_loop_test.cpp      -o C09_Coroutines/exercises/build/c09-review-mini/good_run_loop_test
g++ ... tests/stop_test.cpp          -o C09_Coroutines/exercises/build/c09-review-mini/good_stop_test
g++ ... tests/when_any_test.cpp      -o C09_Coroutines/exercises/build/c09-review-mini/default_when_any_test
g++ ... tests/shared_task_test.cpp   -o C09_Coroutines/exercises/build/c09-review-mini/default_shared_task_test
g++ ... tests/run_loop_test.cpp      -o C09_Coroutines/exercises/build/c09-review-mini/default_run_loop_test
g++ ... tests/stop_test.cpp          -o C09_Coroutines/exercises/build/c09-review-mini/default_stop_test
g++ ... tests/when_any_test.cpp      -o C09_Coroutines/exercises/build/c09-review-mini/bad_when_any_no_drain_test
g++ ... tests/shared_task_test.cpp   -o C09_Coroutines/exercises/build/c09-review-mini/bad_shared_single_waiter_test
g++ ... tests/run_loop_test.cpp      -o C09_Coroutines/exercises/build/c09-review-mini/bad_executor_inline_test
```

上述编译均 exit 0。`...` 仅省略上文 include 顺序或对应 bad/default include 前缀；实际执行命令保留在本轮命令记录中。

## run_test.py 监督结果

每个进程由 `C07_OS_Memory_System_IO/exercises/tools/run_test.py --timeout 30` 监督，JSON 写入 `mini-student-evidence/records/`。

```text
good_when_any-20260911T023809677836Z.json: verdict=PASS exit=0 timeout=False required='when_any:'
good_shared_task-20260911T023809674617Z.json: verdict=PASS exit=0 timeout=False required='shared_task:'
good_run_loop-20260911T023809677857Z.json: verdict=PASS exit=0 timeout=False required='run_loop:'
default_stop-20260911T023809677806Z.json: verdict=PASS exit=0 timeout=False required='provided stop_token aliases'
default_when_any_rejects-20260911T023809677877Z.json: verdict=PASS exit=1 timeout=False required='TODO: implement mini::when_any'
default_shared_task_rejects-20260911T023809677788Z.json: verdict=PASS exit=1 timeout=False required='TODO: implement mini::shared_task'
default_run_loop_rejects-20260911T023828549106Z.json: verdict=PASS exit=1 timeout=False required='TODO: implement mini executor schedule'
bad_when_any_no_drain_rejected-20260911T023828549134Z.json: verdict=PASS exit=1 timeout=False required='winner must cancel and wait for loser'
bad_shared_single_waiter_rejected-20260911T023828549295Z.json: verdict=PASS exit=1 timeout=False required='all shared waiters must resume'
bad_executor_inline_rejected-20260911T023828549056Z.json: verdict=PASS exit=1 timeout=False required='schedule must queue, not resume inline'
```

## 判定

- `when_any_test.cpp` 能由独立 good 通过，并拒绝“winner 后立即恢复父协程、不等待 loser drain”的 bad。
- `shared_task_test.cpp` 能由独立 good 通过，并拒绝“只唤醒一个 waiter”的 bad。
- `run_loop_test.cpp` 能由独立 good 通过，并拒绝“schedule inline resume 或重复/未入队”的 bad。
- 默认 Student 起点对 `when_any/shared_task/run_loop` 都是安全、明确、有限的非零失败。
- 新 `stop_token` alias 观察测试在默认 Student 起点通过。

因此，新补 Capstone5 Student checker 对这三类任务具备可做性和基本拒错能力；未发现需要主代理先修复的签名、危险窗口或测试设计阻断。

## 2026-09-11 补充复验：provided `mini::task` 与 sender 扩展

结论：APPROVE。

本轮复验目标是核对新提供的 `mini/task.hpp` start/单次消费 guard、`task_test.cpp`、可选 `task_sender_test.cpp` 与 `mini_task_sender_test` 接线是否破坏此前三类 checker，并验证 `mini::task` sender TODO 具备独立可做路径。仍不修改生产 Student/Reference/正文；新增候选实现只在本 evidence 目录内。

### 当前源版本

```text
0B7125518D2173FDCEDB894370FCE2BE8DC186635389B5BDA27CFC6B1AC2CF68  C09_Coroutines\exercises\Capstone5_mini_corolib\include\mini\task.hpp
4BF152923B2078AF32FB25804892B551D468D5F296126A82B71DE3B110C948E3  C09_Coroutines\exercises\Capstone5_mini_corolib\tests\task_test.cpp
BBC617AC044DEA8FAD8F5B7CD7476FA151EE5BAA317484E6B36CBB880C14E69C  C09_Coroutines\exercises\Capstone5_mini_corolib\tests\task_sender_test.cpp
958D7C85C83FE9B067442B3EF0B375F5963E635CF6A5479D03AC0D3F5B3F57A6  C09_Coroutines\exercises\Capstone5_mini_corolib\CMakeLists.txt
1467EF727E3239C38619F1A02485287DA57152F7EAD6F9E41B95F3025541344E  mini-student-evidence\good_task_sender\include\mini\task.hpp
967292E935F0C9C0B438B9618AC3F3F2CF270DED7C1B5D519F1B4ACC8246A2B2  mini-student-evidence\bad_task_sender_wrong_value\include\mini\task.hpp
48F458C6BD09DEC589179ACD4CC7CBC06CBBC3F3CC7B1D9FB813B81518170E0A  mini-student-evidence\bad_task_sender_double_complete\include\mini\task.hpp
27693378C957B17F96A0F8379F9B717B8246DADC6B7A3925B6C54044594BCE2D  mini-student-evidence\task-sender-cmake\CMakeLists.txt
stdexec cached source HEAD: 6d7ad689f4d4831c5136e4abe1c601f9a3b64e43
```

### 原三类 checker 复跑

WSL Ubuntu GCC 13.3.0，`-std=c++20 -Wall -Wextra -pedantic`，include 顺序仍为 evidence 覆盖头优先、Student `include/mini` 其次、tests/common include 后置。`mini/task.hpp` 使用当前生产 Student 提供版本。

```text
rerun_good_when_any-20260911T033955615563Z.json: PASS exit=0 contains='when_any:'
rerun_good_shared_task-20260911T033955881888Z.json: PASS exit=0 contains='shared_task:'
rerun_good_run_loop-20260911T033956238563Z.json: PASS exit=0 contains='run_loop:'
rerun_bad_when_any_no_drain_rejected-20260911T033956472836Z.json: PASS exit=1 contains='winner must cancel and wait for loser'
rerun_bad_shared_single_waiter_rejected-20260911T033956665215Z.json: PASS exit=1 contains='all shared waiters must resume'
rerun_bad_executor_inline_rejected-20260911T033956862414Z.json: PASS exit=1 contains='schedule must queue, not resume inline'
```

判定：新提供的 `mini::task` start/消费 guard 未让此前 when_any/shared_task/run_loop checker 误拒绝；三类独立 good 仍通过，三类行为 bad 仍被拒绝。

### task sender 独立可做性

新增 review-only 候选：

- `good_task_sender/include/mini/task.hpp`：复制 Student 基础 `task<T>/task<void>` 形状，补最小 `sender_concept`、`completion_signatures`、member `connect(receiver)` 与 `operation_state`；`connect` 不启动，`stdexec::start(op)` 后同步完成 value/error 各一次。
- `bad_task_sender_wrong_value/include/mini/task.hpp`：行为型坏实现，错误发送默认值，验证 checker 能拒绝错值。
- `bad_task_sender_double_complete/include/mini/task.hpp`：行为型坏实现，同一 operation 发送两次 value，验证 checker 能拒绝双完成。

验证采用自有薄 CMake 项目 `mini-student-evidence/task-sender-cmake`，`add_subdirectory` 加载真实 cached stdexec 源树，生成并链接 `stdexec::stdexec`。MSVC 19.51 / Visual Studio 18 2026 Release 构建通过：

```text
cmake -S mini-student-evidence/task-sender-cmake -B C09_Coroutines/exercises/build/c09-review-mini-task-sender -DSTDEXEC_SOURCE_DIR=F:/CPPTrain/LearnCPP/C09_Coroutines/exercises/build/full-windows/_deps/stdexec-src
cmake --build C09_Coroutines/exercises/build/c09-review-mini-task-sender --config Release --target task_sender_default task_sender_good task_sender_bad_wrong_value task_sender_bad_double_complete
```

`.vcxproj` 回读显示 good/bad target 的 `AdditionalIncludeDirectories` 以对应 `mini-student-evidence/*/include` 开头，其后才是生产 Student include；stdexec 通过 `F:/CPPTrain/LearnCPP/C09_Coroutines/exercises/build/full-windows/_deps/stdexec-src/include` 和 `_stdexec-build/include` 接入。对候选、薄 CMake、`task_sender_test.cpp` 扫描 `mini_ref|reference/include|Capstone5_mini_corolib/reference|#include.*reference` 无命中。

`run_test.py --timeout 30` 结果：

```text
task_sender_default_todo_rejects_r2-20260911T034202134970Z.json: PASS exit=1 contains='TODO: implement mini::task sender metadata and member connect'
task_sender_good_r2-20260911T034202894740Z.json: PASS exit=0 contains='task sender: deferred start, value/error, one completion checked'
task_sender_bad_wrong_value_rejected_r2-20260911T034203511239Z.json: PASS exit=1 contains='task sender value mismatch'
task_sender_bad_double_complete_rejected_r2-20260911T034204134352Z.json: PASS exit=1 contains='task must complete through one channel'
```

判定：默认 Student 的 `mini_task_sender_test` 是明确 TODO 失败；独立 sender good 可通过同一测试；错值和双完成两个行为 bad 被精确拒绝。这个 good 只证明教学检查的最小可做性，不代表完整异步生产级 `task` sender 实现。

### 边界

第一次 task sender 薄 CMake 配置把 overlay 写成了 `task-sender-cmake/good_task_sender` 子路径，导致 good/bad 误编到默认 Student 并产生 3 条 FAIL 记录；随后已修正为 sibling overlay 路径并用 `_r2` 记录复跑通过。最终验收应采用本节列出的 `_r2` 记录。
