# 同步批次：Starter 修订交接与验证记录

日期：2026-09-08。用户传达的上轮独立 review：公共实现未发现确定 P0/P1；教学入口有 P1（main 直接包含答案）与 P2（只测试 Reference）两个阻断。本次已按这两项修订，等待同一独立 reviewer 复验；本文不把作者验证表述为独立复验通过。

本作者所有编辑仍限 Concurrency_Study，使用 apply_patch，未提交 Git，未编辑任何 CMake、根 README、公共 benchmark 或覆盖/标准/质量索引。主线程新增的 CONCURRENCY_STUDY_TEST_STARTERS 接线已直接用于本次验证。ponytail 技能促使两种实现复用必要的契约检查；学生实现与答案独立，未建立通用测试框架。

## 两项阻断的处理

十个 main.cpp 均改为独立 Starter：每 Part 有具名学生函数或学生方法、具体输入与契约、局部中文提示，以及实际调用学生代码的检查。已有的局部观察示例明确标注。对应 part*_done 默认为 false；启动保护在对象/线程创建之前输出 STARTER INCOMPLETE 并返回 1。标记仅解除保护，不能替代检查；只改标记而保留 TODO 会失败，错误等待也可能被外部超时终止。

C2 main.cpp 定义 student_channel<T>，传给 channel_checks::run<student_channel>()；solution.cpp 则传 cs::bounded_channel。Capstone1 main.cpp 定义 student_pool，传给 pool_checks::run<student_pool>()；solution.cpp/runtime_tests/thread_pool_test.cpp 传 cs::thread_pool。两个 checks.hpp 都不包含答案实现，只调用其类型参数。Capstone1 允许复用已学的 bounded_channel，池的构造、worker、提交、关闭与寿命逻辑仍由学生实现。

八个 API 观察题的学生接口和 Part 映射见各 README。B2 等待图与 C1 文字推导的非空检查只确认已填写，正确性仍需人工复盘。所有 main 均不再 include solution.cpp；Reference 中的危险路径继续双重隔离，Starter 不执行危险演示。

每题 README 已分别列出以下两套完整命令（下列 ID 在每题文档中都替换成具体题名，工作目录为 exercises）：

```powershell
# 只验证完整答案，默认不注册学生测试
cmake -S ID -B build/reference-ID -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/reference-ID --config Release --target ID_reference
ctest --test-dir build/reference-ID -C Release -R "^ID_reference$" --no-tests=error --output-on-failure

# 实际运行 main 中的学生实现
cmake -S ID -B build/student-ID -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-ID --config Release --target ID
ctest --test-dir build/student-ID -C Release -R "^ID_student$" --no-tests=error --output-on-failure
```

未完成的学生测试报 Failed 是预期，未设置 WILL_FAIL 或 SKIP 来反转结果。默认 OFF 的 Reference 门禁不受它影响。每题还提供 `python tools/run_diagnostic.py --timeout 5 -- EXE` 的实际路径命令。四章与线程池专题同步区分两种入口，已去掉“从答案重建入口”和“main 包含答案”的过时描述。

## 公共集成 API（本次未改变）

- `cs::bounded_channel<T>(capacity)`：`bool push(T)`、`optional<T> pop()`、`void close()`。MPMC、固定正容量、阻塞 FIFO、close 拒收并排空；元素无抛出移动构造/析构，不可重入；所有调用者先结束再析构。
- `cs::thread_pool(workers, capacity)`：两个参数均为正；`submit(F&&, A&&...) -> future<invoke_result_t<decay_t<F>, decay_t<A>...>>`、幂等 `shutdown()`。外部并发提交/关闭、move-only、异常 future、拒收、广播、排空、join。
- 同池 worker 调用 submit/shutdown 抛 logic_error；worker 自销毁禁止、析构 fail-fast。跨池或外部 future 依赖仍可能死锁。公开契约只有 drain，没有 cancel-pending。
- 公共头 SHA256 与上轮清单一致。新增两个本题 checks.hpp 是头文件，不需要定制多源/库链接；继续使用主线程现有 C++23/Threads 接线。runtime 入口改为包含本题检查头，不再充当被其他 cpp 包含的 main。
- 下表共 44 个课程产出文件（原 42 + 两个检查头），加本文共 45 个。临时验证副本仅位于已忽略的 exercises/build/sync-starter-probes，不属于学生交付或公共实现。

## 本机验证

环境：Windows x64，MSVC 19.51.36256.0，Visual Studio 18 2026 BuildTools，Windows SDK 10.0.26100.0。叶项目由公共 CMake 请求 C++23（本机生成的 VS 项目使用 stdcpplatest）；临时填充的 API Starter 另以 /std:c++23preview 编译。Release 未完成分支和明确抛出 TODO 会产生 MSVC C4702（不可达代码）警告；无编译错误，不宣称零警告。

| 题目 | Reference 独立门禁 | 原样 student CTest | 5 秒外部有界运行 |
|---|---|---|---|
| B1_mutex_family | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| B2_deadlock_scoped_lock | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| B3_call_once | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| C1_condvar_predicate | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| C2_bounded_queue_condvar | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| C3_interruptible_wait | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| A2_stop_token_cancellation | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| H1_latch_barrier | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| H2_semaphore | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |
| Capstone1_thread_pool | PASS | Failed（预期 INCOMPLETE） | 退出 1，无超时 |

实际逐题执行了 build/reference-ID 的 OFF 配置、构建和 Reference CTest，并用 CTest JSON 确认各配置没有注册 *_student。逐题执行 build/student-ID 的 ON 配置，Reference 与 Starter 均成功构建；Reference 全部通过，学生测试均因启动保护失败。用 CTest JSON 确认十个 *_student 各自 TIMEOUT=30；另外逐题通过现有 run_diagnostic.py --timeout 5 验证实际入口退出 1，未超时。

其他验证：

- runtime_thread_pool_test：抽出检查头后重新构建并通过，目录 exercises/build/sync-author-runtime。
- 类型分派探针：build/sync-starter-probes/dispatch.cpp 不包含公共 channel/pool 头，传入构造时抛出唯一标记的探针类型；两个 run 都到达该类型并由探针捕获，运行 PASS。这验证检查没有暗中固定使用 cs 答案。
- 八个 API Starter 临时填充验证：只在 build/sync-starter-probes/completed-ID.cpp 副本内填入标准实现并解除标记，B1/B2/B3/C1/C3/A2/H1/H2 全部在 5 秒外部超时内通过自己的学生检查。交付的 main.cpp 保持 TODO 和 false 标记；这些结果不声称学生已完成练习。A2 的 inplace 专项仍为 SKIP。
- C2/Capstone1 的类型参数化正确性检查通过公共实现实例化运行，另由分派探针确认可替换类型。未另写一套已完成的学生 channel/pool，以免把 Starter 再变成答案。
- 本次更改的 15 个正文/题面相对链接检查：断链 0；十个 README 均有带精确 student/reference 正则的独立命令。
- 分配范围 git diff --check 通过；未覆盖其他作者变更。

## 未测与复验安排

- 本次未做 TSan、跨架构/跨编译器或性能测量；未运行实际 UB、永久死锁或 worker 自销毁路径。上轮危险选项隔离结果保留为历史记录，本次 Starter 不包含这些诊断。
- C++26 inplace 分支缺能力，未编译/运行。宏为 0 时 A2 只要求完成普通 stop_token Parts；规范仍固定 N5050。
- 公共池部分线程创建失败回滚与底层互斥/条件变量设施故障仍未注入；不以零参数拒绝或 Starter 前置退出冒充该验证。
- 临时填充验证支持本次接口与检查能组成可运行答案，不证明任意学生实现或所有交错；错误协议仍可能等待至 CTest 超时。
- 请同一独立 reviewer 针对下列文件版本复验 P1/P2：逐 Part 学生入口是否明确、默认是否线程前非零退出、检查是否调用学生对象、两套命令是否真正区分 main 与 Reference。原公共算法文件未变，但检查调用关系已改变，应一并核验。

## 产出版本清单

路径相对 Concurrency_Study；SHA256 对应原始文件字节，不对本文做自哈希。

| 文件 | SHA256 |
|---|---|
| [chapters/04-shared-state-and-locks.md](../../chapters/04-shared-state-and-locks.md) | `5BCFC1728F7EF636E2D602A2BCFE77D734DB02EA33862859CDDB02DD2F941931` |
| [chapters/05-waiting-and-channels.md](../../chapters/05-waiting-and-channels.md) | `859BE6014E0BE9FFCAFAFB61A7B52C6A4EA2CE94A4DDF5739F914BEC50FD5E29` |
| [chapters/06-cancellation-and-shutdown.md](../../chapters/06-cancellation-and-shutdown.md) | `1A409A5D83734DACF3183305BD8FC5C5EFAB712B7C12736149AD8BD16D42AF96` |
| [chapters/07-coordination.md](../../chapters/07-coordination.md) | `6E4BCD12FA988762DEC5BC9D9E403DF5C53524745FC47B83EB460607DB03E51C` |
| [topics/synchronization/01-bounded-thread-pool.md](../../topics/synchronization/01-bounded-thread-pool.md) | `68E8530E92DCAA1275B2D13182F457891B13DF3F0B36ABAF88DE8743FAD2EAE2` |
| [exercises/B1_mutex_family/main.cpp](../../exercises/B1_mutex_family/main.cpp) | `A5E51C5C443B108510CD05776E2752CB203C65A86DC0D1F656A0C268DD21075A` |
| [exercises/B1_mutex_family/solution.cpp](../../exercises/B1_mutex_family/solution.cpp) | `E69C0B9C9041F46883056DE4291A20457A4C107E1E0F42F58045A5DFB1A78C30` |
| [exercises/B1_mutex_family/README.md](../../exercises/B1_mutex_family/README.md) | `93E5FA0AAE8D1AC63BE72CA5BF75C3158F973C17E2F4CB592288E9D8F2857BBE` |
| [exercises/B2_deadlock_scoped_lock/main.cpp](../../exercises/B2_deadlock_scoped_lock/main.cpp) | `C89FE9FA16CE1C92BF7F8EA4BF65DC9CA5BDDC0ED9B23BDA63EF500A206389AD` |
| [exercises/B2_deadlock_scoped_lock/solution.cpp](../../exercises/B2_deadlock_scoped_lock/solution.cpp) | `202C30FEB912821CF8864DB8F05F95EB495A32550ADF3D0EE0DFCDB791F97AC9` |
| [exercises/B2_deadlock_scoped_lock/README.md](../../exercises/B2_deadlock_scoped_lock/README.md) | `50D898CBE9401FC982528F85F51B986E4CD210050D8B97104291E6716079E339` |
| [exercises/B3_call_once/main.cpp](../../exercises/B3_call_once/main.cpp) | `4000E6AA2935F8FBBE1B03BFA65F8628044D700E76E1CCF4B631AB6C608D65A1` |
| [exercises/B3_call_once/solution.cpp](../../exercises/B3_call_once/solution.cpp) | `D3CD74B090ADF4D260AD70AFF740D0342452E8D278D4AE239A3D424E5D20190D` |
| [exercises/B3_call_once/README.md](../../exercises/B3_call_once/README.md) | `9889921B51C9149C7CE31ABA6AF149CA5BC59B6848F0DBB39E8B9CAACF1A9696` |
| [exercises/C1_condvar_predicate/main.cpp](../../exercises/C1_condvar_predicate/main.cpp) | `E285F3A70BE9A206A19D8F5CF7815DD648FD024128CF1A81292F66249DAA93C1` |
| [exercises/C1_condvar_predicate/solution.cpp](../../exercises/C1_condvar_predicate/solution.cpp) | `31FFC25184433EE4CADF868E92870FCF64B45654A479A49590BA5A79699B1F9A` |
| [exercises/C1_condvar_predicate/README.md](../../exercises/C1_condvar_predicate/README.md) | `02D64AFEABADA2F5D58F439BB3205C721F0C418D3725E1DC0C27032D23B3C9D4` |
| [exercises/C2_bounded_queue_condvar/main.cpp](../../exercises/C2_bounded_queue_condvar/main.cpp) | `C5D2FEB12C4018908194EA93977FCB452D18E20CDBC2BFD4D36D023353F99168` |
| [exercises/C2_bounded_queue_condvar/solution.cpp](../../exercises/C2_bounded_queue_condvar/solution.cpp) | `65634D8A5E94E11639E00B502FF8422FBF6046A3324C7287C5FF5788C3013AF1` |
| [exercises/C2_bounded_queue_condvar/README.md](../../exercises/C2_bounded_queue_condvar/README.md) | `D68F390A43B514836C08A55A65971257AF665EEF1FC3FB5AA93A2DD1E62CF5C2` |
| [exercises/C3_interruptible_wait/main.cpp](../../exercises/C3_interruptible_wait/main.cpp) | `28C900873541D9C442590BFBB77EF5E973D9152C13638523D3881C71F7FF8E03` |
| [exercises/C3_interruptible_wait/solution.cpp](../../exercises/C3_interruptible_wait/solution.cpp) | `73E13DCCD6A6BF02A28166B5D101EF1D588CDEFC9B5387E596D1BB78B5169FF3` |
| [exercises/C3_interruptible_wait/README.md](../../exercises/C3_interruptible_wait/README.md) | `2F907EBB2BC21DB59CD87AF86892EAF0E9A96C33AEF39C72BAA3F8D8528CBE7C` |
| [exercises/A2_stop_token_cancellation/main.cpp](../../exercises/A2_stop_token_cancellation/main.cpp) | `6F45659AF92522FF692FA8CABDA0F472CAEFEA8ED80B54EA2CF4BBC6437F231A` |
| [exercises/A2_stop_token_cancellation/solution.cpp](../../exercises/A2_stop_token_cancellation/solution.cpp) | `1989B69BE338BF37921FAF79F19ACEB043CBE65D78A39BA32278EB50D6A82993` |
| [exercises/A2_stop_token_cancellation/README.md](../../exercises/A2_stop_token_cancellation/README.md) | `1C216D63792E8635A739377D1282649CEF3F5F3DCE2498186E4099710BE77DF9` |
| [exercises/H1_latch_barrier/main.cpp](../../exercises/H1_latch_barrier/main.cpp) | `25C74A42546B1CF102F873AB25C02B1244D6C37B934261BC1CE009DEDF2F047F` |
| [exercises/H1_latch_barrier/solution.cpp](../../exercises/H1_latch_barrier/solution.cpp) | `8348F167AF337944A5487CDF9611C9798FA41D9D3A473CA5F8E09923A2E19DD2` |
| [exercises/H1_latch_barrier/README.md](../../exercises/H1_latch_barrier/README.md) | `3F6ADF0AE3FF496EF040FD974894DC48E57DADF8059966DB56D51138E13DE900` |
| [exercises/H2_semaphore/main.cpp](../../exercises/H2_semaphore/main.cpp) | `3DE27260BC0070AF114E3F8BF27532FA365B9D5D4D4496AB8B92D480F637849A` |
| [exercises/H2_semaphore/solution.cpp](../../exercises/H2_semaphore/solution.cpp) | `754EE3D0D72E7CF06225757043324224822972FB00A7B6C9D4DDF68424A887B5` |
| [exercises/H2_semaphore/README.md](../../exercises/H2_semaphore/README.md) | `34EFC1BB33E6813FCD40F0532B762DF9E122B1990E597F76DCD7EAE341849A98` |
| [exercises/Capstone1_thread_pool/main.cpp](../../exercises/Capstone1_thread_pool/main.cpp) | `8F118C56EFA400D40D40107A15A2F6A6637DD501A5AF3A6761FF909AD6A42E34` |
| [exercises/Capstone1_thread_pool/solution.cpp](../../exercises/Capstone1_thread_pool/solution.cpp) | `BAF57F7193A2BEBAA2D74BEDC0FB569D22E396016789D503DE99555430C393E1` |
| [exercises/Capstone1_thread_pool/README.md](../../exercises/Capstone1_thread_pool/README.md) | `D7D2200670BEC0AA14D600B61C08AB98E22F3E278C586205000F06F0F29A4DBC` |
| [exercises/include/concurrency_study/bounded_channel.hpp](../../exercises/include/concurrency_study/bounded_channel.hpp) | `280DBA008A6B5C60E480D581D740FEC1472D6B863E1E13D44B68D0ED80CB3385` |
| [exercises/include/concurrency_study/thread_pool.hpp](../../exercises/include/concurrency_study/thread_pool.hpp) | `3AA83128F8C195858C7333695DFDEAE309CE44D9C20C1707657EEF86228F766A` |
| [exercises/runtime_tests/thread_pool_test.cpp](../../exercises/runtime_tests/thread_pool_test.cpp) | `02AE00EA75A05C3CC45AB48D813C79E2F0289B92171019511C3367D167C40812` |
| [03-模块B-互斥与锁.md](../../03-模块B-互斥与锁.md) | `804AE18D312EE4359600422C6B0C5137AF5461879542A7F1CDEFEA84E29D17B4` |
| [04-模块C-条件变量.md](../../04-模块C-条件变量.md) | `82D51A92AFE46F41F166C5F0BCFA3E4D9955DF4650D87F0FC44ADB6FCCF52195` |
| [06-第一阶段结课-线程池与生产者消费者.md](../../06-第一阶段结课-线程池与生产者消费者.md) | `85BF1146663A6CECF0555F11BC79E8A6F18E433A393080DCFDA6430DA7434D96` |
| [10-模块H-高级同步原语.md](../../10-模块H-高级同步原语.md) | `3E429BE4EFD151C2AD775978662D5A3C59CC338CB2A7A9A915B8AC58157C6431` |
| [exercises/C2_bounded_queue_condvar/checks.hpp](../../exercises/C2_bounded_queue_condvar/checks.hpp) | `59A4A412BA01279856DBB5A9611A707A200F06A1EE7EB6B6C855C53C4BC85884` |
| [exercises/Capstone1_thread_pool/checks.hpp](../../exercises/Capstone1_thread_pool/checks.hpp) | `3042197E39A26F5A669048B64B436FF8A159A9BBCC5B9E08001FE3D1ED807708` |
