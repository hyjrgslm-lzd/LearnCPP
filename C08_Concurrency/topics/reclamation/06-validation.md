# 回收专题：复现、验证范围与集成交接

这里记录本专题代码的作者验证，不替代后续非作者独立 review。所有普通实验都使用 cs::check，Release 下检查仍执行；worker 中需要传回的异常由 std::async/future.get 送到主线程。没有把故意错误的回收路径放进性能比较。

## 1. 当前复现：区分学生入口与只读答案

四题 main 现在是独立学生 starter，空作业预期返回 1，solution 预期返回 0；不可把二者链接进同一个程序。学生只编辑 student.hpp。按本轮教学试做反馈，专题 verify.ps1 已将参考验证与学生验证分开；未改公共 runner、CMake 或索引。

```powershell
./C08_Concurrency/topics/reclamation/verify.ps1                    # 默认 Reference：6 个程序应返回 0
./C08_Concurrency/topics/reclamation/verify.ps1 -Mode Starter      # 四个空 main 应返回 1，且未启动 worker
./C08_Concurrency/topics/reclamation/verify.ps1 -Mode Student      # 四个补全 main 必须实际检查通过，返回 0
./C08_Concurrency/topics/reclamation/verify.ps1 -Asan              # 默认 Reference 的 ASan 验证
```

Starter 模式还要求诊断含 TODO，且没有启动 worker 的日志；Student 模式必须出现 student PASS。未完成 main 传给 Student 模式会使脚本失败，不能因它在 Starter 模式符合预期而认定作业完成。

从仓库根目录的 x64 MSVC Developer PowerShell，分别编译和运行一题（其他题换路径与输出名）：

```powershell
New-Item -ItemType Directory -Force C08_Concurrency/topics/reclamation/.build | Out-Null
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG /IC08_Concurrency/exercises/include /FoC08_Concurrency/topics/reclamation/.build/I2_student.obj /FeC08_Concurrency/topics/reclamation/.build/I2_student.exe C08_Concurrency/exercises/I2_hazard_pointer/main.cpp
./C08_Concurrency/topics/reclamation/.build/I2_student.exe
$LASTEXITCODE # 空 starter 为 1；全部学生检查通过后才为 0
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG /IC08_Concurrency/exercises/include /FoC08_Concurrency/topics/reclamation/.build/I2_reference.obj /FeC08_Concurrency/topics/reclamation/.build/I2_reference.exe C08_Concurrency/exercises/I2_hazard_pointer/solution.cpp
./C08_Concurrency/topics/reclamation/.build/I2_reference.exe
```

ASan 验证增加 /fsanitize=address /Z7，运行进程需能从 PATH 找到对应工具链的 ASan DLL；专题脚本会显式传入该路径，各模式都可加 -Asan。C++ 语言目标是 C++23；c++23preview 是本机 MSVC 接受的相应选项。本次 33 个独立变体使用专题 .build/student-paths 私有目录，每个进程外部超时 15 秒；专题正式验证脚本为每进程 30 秒，不混为同一轮证据。具体结果见第 7 节。

单独验证时可以在 Visual Studio x64 Native Tools Command Prompt 中运行（当前目录是仓库根）：

```bat
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG ^
 /IC08_Concurrency/exercises/include ^
 /FoC08_Concurrency/topics/reclamation/reclamation_test.obj ^
 /FeC08_Concurrency/topics/reclamation/reclamation_test.exe ^
 C08_Concurrency/exercises/runtime_tests/reclamation_test.cpp
C08_Concurrency\topics\reclamation\reclamation_test.exe
```

把输入路径换成某题的 solution.cpp 或本目录 lifetime_reference.cpp，就得到对应独立程序；同时给输出选择不同文件名。GCC/Clang 的预期形式如下，但本批未在这两种工具链运行，不把命令示例标记为验证通过：

```sh
c++ -std=c++23 -O2 -DNDEBUG -pthread -IC08_Concurrency/exercises/include \
  C08_Concurrency/exercises/runtime_tests/reclamation_test.cpp \
  -o C08_Concurrency/topics/reclamation/reclamation_test
```

需使用含 std::move_only_function 的 C++23 标准库。I3/R1/R2 及 runtime 的实验路径另包含本目录 experiment_support.hpp，打包这些 reference 时须一并保留；不需要额外 include 目录或链接库。这里没有额外第三方依赖、后台服务或平台专用运行库链接要求；Unix 侧线程支持通常通过 -pthread 配置。

## 2. 逐项检查到底证明了什么

| 场景 | 对应代码/函数 | 必须观察到的结果 |
|---|---|---|
| 固定延迟仍不足、标签不保护生命期 | lifetime_reference.cpp | 安全模型达到反例状态，不真正解引用悬垂指针 |
| 强所有权延长旧版本生命期 | reference_count_branch | 清源后不析构；最后强引用释放后恰好一次析构 |
| HP 保护与退休线程退出 | hp_handles_and_exit | 退休线程结束后记录仍在；有效保护期间 0 次删除 |
| HP 移动、reset、槽耗尽 | 同上 | 移动只解除目标旧保护；reset 保留槽；已退休对象可从有效保护转交；128 个槽满时报 bad_alloc，释放后可复用 |
| HP 删除器状态与重入 | hp_state_and_reentry | 只能移动的状态保留，7+11=18；父子恰好两次回调，嵌套 cleanup 返回 0 |
| RCU 嵌套、多域、短命线程 | rcu_nested_domains_and_exit | 内层退出不结束外层；另一域可完成；32 次线程退出后可同步与清场 |
| RCU 删除器状态、派生退休 | rcu_state_and_callback_reentry | 第一轮只处理固定批次，第二轮处理 child；禁止回调自等待 |
| 并发 barrier 与回调结束 | concurrent_barriers | B 等待 A 已取走的回调，返回后读到回调写入的普通值 17 |
| 晚退休批次 | late_retirement | 后进入读者保护的对象不使用旧批次的宽限期 |
| 新代不拖旧记录 | later_generation_does_not_block | 退休后才进入的 guard 不阻挡这条旧退休记录 |
| 长 RCU 读者与 synchronize | synchronize_waits_for_old_reader | 读者未退出时等待；退出后 synchronize 不运行回调，barrier 才删除 |
| QSBR 注册与上下线契约 | qsbr_contracts_and_offline | 混用/重复注册报错；offline 解除阻挡；重新 online 和 quiescent 正常 |
| HP 栈 | hp_exercise::run | 混合阶段 4000 个 ID 逐个恰好一次、4000 次析构；另用两个节点检查 LIFO/空栈 |
| 不可变配置 | rcu_exercise::run | 8000/8000 次读取全部检查、65 次析构、无 pending |
| EBR 长读者 | ebr_exercise::run | 64 份退休积压、退出后回收 64、最终总数 65 |
| QSBR 漏报告 | qsbr_exercise::run | 已读完但未报告仍保留 64；报告后在线状态下回收 64、最终总数 65 |
| 注册、分配、报告、回调与启动失败 | failure_paths | 21 个定点异常均保留原类型/阶段；启动数=完成数；创建数=销毁数；域正常析构 |

门闩负责制造确定的“保护已建立”和“回调已开始”阶段。关于阻塞的 40 ms wait_for 是有限调度采样，不能保证启动通知后的线程立即执行到了等待函数；因此测试不是穷举证明。SC/元数据锁/批次归属的推导写在正文，动态检查用于发现实现偏差。

## 3. 本机观测与未验证项

2026-09-08，Windows x64，MSVC 19.51.36256（VC 工具目录 14.51.36231），PowerShell 7.6.5：

- 普通 Release：上述十个程序全部编译和运行通过，/W4 /WX 没有剩余警告。
- ASan Release：相同十个程序全部编译和运行通过，未报告地址访问错误。
- 共享头兼容性追加检查：其他作者新增的 G1_treiber_stack/solution.cpp 与 Q2_ms_queue/solution.cpp 也使用最终 HP 头，以相同 /W4 /WX、优化及 ASan 选项单独编译运行通过。已阅读其 queue_linked.hpp 调用，源指针为 SC、析构清场调用 cleanup；未修改这些客户端文件。此结果是编译/运行兼容性证据，不是对其他作者数据结构的独立 review。
- 生命周期 reference 的 atomic<shared_ptr>.is_lock_free 输出为 0。它只描述本机实现，不据此推断所有平台或给方案排性能名次。
- R1 的 64 份 int 退休载荷为 256 字节。它不是总堆内存测量，未包括分配器与回收记录。
- 未运行 TSan、LSan、ARM/弱内存硬件、GCC/Clang 或模型检查器。ASan 通过不等于数据竞争检测通过，也不等于证明不存在全部泄漏。
- 没有执行真实 UAF、double delete、强制线程退出、代号溢出、内存耗尽或抛异常删除器的进程死亡测试；相应前置条件与终止边界通过代码审查和正文明确，未把它们标成运行验证通过。
- 无性能基准结论。本批未测 lock-free 进展、公平性、最大延迟、吞吐排名或实际总堆峰值。
- CMake 总集成及第 10 章入口由主线程负责；本专题通过直接编译验证，不以其他作者正在修改的构建状态作为完成依据。P2 协议修复已获 APPROVED，历史证据见第 6 节；新学生路径及待复验状态见第 7 节。
- 首轮交付时本专题及练习的 12 份 Markdown 已检查本地链接；当时唯一尚未存在的目标是迁移入口中的 chapters/10-publication-and-lifetime.md，由主线程接线。此为首轮记录，不代表当前章节集成状态。

## 4. 交给课程主线程的构建信息

四个练习目录为 I2_hazard_pointer、I3_rcu、R1_epoch_reclamation、R2_qsbr。main.cpp 包含本题 checks.hpp，检查器调用 student.hpp 的操作；solution.cpp 仍只包含 reference.hpp。Reference 也被 runtime test 包含，不受学生修改影响。不应把这些头当成单独编译单元，也不要同时链接同题的两个 main。没有新增链接库或公共 CMake 改动。

新增 runtime_tests/reclamation_test.cpp 含独立 main。只需公共 exercises/include 和线程支持；相对 include 会找到四题 reference.hpp。新增头 epoch.hpp 是 rcu.hpp 的依赖，HP 自包含。三个头没有依赖 log.hpp、公共 benchmark 工具或 CMake 特性探测。

新增 topics/reclamation/lifetime_reference.cpp 含独立 main，可接入课程的 runtime/reference 目标。无需实现 std::hazard_pointer 或 std::rcu；整个专题使用 C++23 cs 教学实现。

公开新增 API 是 `std::size_t cs::hazard_pointer_cleanup() noexcept`。返回本次已完成回调数，0 不是“全局无退休”的通用判据。epoch_domain 提供 lock/unlock、retire、collect、pending、synchronize、barrier；rcu_domain 是它的教学别名。现有 cs::make_hazard_pointer、hazard_pointer_obj_base、rcu_default_domain、rcu_reader、rcu_obj_base::retire、rcu_retire、rcu_synchronize、rcu_barrier 使用方式保留；显式域是最后一个参数。

请由主线程把 chapters/10-publication-and-lifetime.md 链接到本专题入口；旧 11-模块I-安全内存回收.md 已转换为迁移入口。覆盖/标准/质量索引应标出本批还补齐了 EBR、QSBR、回调完成以及标签与生命期的区别。cs 与 std 不可只换命名空间迁移：本版没有实现所有标准成员，例如标准 HP 的 swap、RCU 域的 try_lock；异常/SC/域构造约束也不同。

## 5. 本作者变更文件清单

本批只改独占范围，未编辑 CMake、根 README、公共 benchmark/索引或公共检查头，未提交 git。其他作者在共享工作树内产生的修改保留原样。

| 范围 | 文件 |
|---|---|
| 公共回收头 | exercises/include/concurrency_study/hazard_pointer.hpp、rcu.hpp；新增 epoch.hpp |
| 既有题 I2/I3 | 各题 main.cpp、solution.cpp、reference.hpp、README.md |
| 新题 R1/R2 | 各题 main.cpp、solution.cpp、reference.hpp、README.md |
| 运行检查 | exercises/runtime_tests/reclamation_test.cpp |
| 迁移入口 | 11-模块I-安全内存回收.md |
| 连续正文 | topics/reclamation/README.md、01-lifetime-and-ownership.md、02-hazard-pointers.md、03-epoch-reclamation.md、04-qsbr.md、05-rcu.md、本文件 |
| 专题验证资产 | topics/reclamation/lifetime_reference.cpp、verify.ps1、.gitignore；P2 返修新增 experiment_support.hpp |

核心设计沿用 ponytail 的最少机制原则：三个区域协议共用可审查的分代核心、删除器用 C++23 move_only_function 保存、转移退休记录用 list::splice。这个选择减少重复协议，但不会删掉边界证明、复杂路径的实际检查或完整中文自测答案。

## 6. P2 复审修复：故障注入与收尾证据

非作者指出的确定问题成立：worker 的注册可能在 entered.set_value 之前抛异常，异常留在 worker future，主线程却对自己仍持有的 entered promise 的另一端永久 wait。另一个环发生在主线程分配失败时：future 析构等待 worker，worker 等 release/report/leave，而持有这些 promise 的主线程还没有走到它们的析构。正常运行、全 SC 或 ASan 没有报错都不能排除这两条异常路径。

本次按等待依赖重新检查了全部作者范围中的异步入口，覆盖 R1、R2、I3，以及 runtime 中的 concurrent_barriers、late_retirement、synchronize_waits_for_old_reader。I2 与其他没有父线程控制门闩的 runtime worker 不具有这个“先等父线程放行，父线程又在析构 future”的环；I2 的任务容器在 stack 前析构，已有任务结束后 stack 才清理节点，未改动其算法。I3 虽无放行门闩，仍补齐启动/写者分配失败后的当前配置与退休记录清理。

### 通知、放行、资源回收各自承担什么

实验辅助头 experiment_support.hpp 仅包含小型 signal/gate、定点注入和对象/线程观测，不改变三个公共回收头或协议。signal 将 promise 移给 worker；主线程仅保留 future，并调用 get 而不是 wait。worker 的 catch 给尚未满足的通知设置原 exception_ptr，再抛回 worker future。R2 同时处理 entered 和 reported，后一个阶段失败也不会让主线程永久等通知。

回调开始通知有一点不同：退休回调与执行它的 worker 共同持有 signal，主线程没有发布端。若 worker 在调用回调之前失败，它先给 signal 设置异常；退出收尾时仍可运行已排队的回调，signal.ready 对已满足的通知不再重复写入。这些发布动作在 worker 内顺序发生，或在主线程已经 wait 该 worker 后执行；signal 不是多发布者并发队列。

控制 gate 只存原子布尔状态，open 可重复且不会发生 promise_already_satisfied。每个有门闩的路径在任何启动动作之前就准备好 future 句柄和 finish 闭包，把可能抛异常的启动/分配动作放入 try。finish 先打开所有相关门，再 wait 所有仍有效的 future；不会先等待其中一个线程、随后才打开另一个线程需要的门。catch 使用 wait 收束其他任务，避免另一份 worker 异常替换当前正在传播的原异常；清理后用 throw 原样重抛。

对象分成三个有真实代码对应的所有权阶段：

| 阶段 | 收尾责任 |
|---|---|
| 已分配但还没发布的 fresh | unique_ptr 在异常展开时销毁 |
| source/current 仍发布的最后版本 | 全部 worker 收束之后 exchange(nullptr)，由主线程直接删除 |
| 已经 retire 的旧版本/回调 | 由域的 barrier 排空，不能直接重复 delete |

正常路径仍把最终版本 retire 后排空；直接删除当前值仅用于通用 finish 中尚有当前值的异常收尾。观测对象额外持有 audit 引用，用于统计实际创建/销毁；R1/R2 的 256 字节输出仍只是 64 份 int 的逻辑载荷，不包括观测字段及填充。

### 注入场景与检查结果

failure_paths 直接调用真实 reference/测试函数，向选定位置抛出带 point 字段的 injected_bad_alloc（继承 std::bad_alloc）。没有替换全局 new，没有真的耗尽内存，没有修改 epoch 注册算法。注册前注入模拟“尚未建立读区、尚未发送启动通知就失败”；分配前注入模拟调用 new 前的异常出口。未发布值、报告、二次启动及回调前失败覆盖相邻的收尾分支。

| 调用路径 | 注入点 | 预期创建/销毁对象数 | 预期启动/完成 worker 数 |
|---|---|---:|---:|
| R1 | 注册前 / 第九次更新分配前 / 第九份新值发布前 | 1 / 9 / 10 | 各 1 |
| R2 | 注册前 / 第九次更新分配前 / 第九份新值发布前 / 报告前 | 1 / 9 / 10 / 65 | 各 1 |
| I3 | worker 注册前 / 第九次更新分配前 / 发布前 | 65 / 9 / 10 | 各 4 |
| concurrent_barriers | 第一个回调前 / 第二个 worker 启动前 | 各 1 | 各 1 |
| late_retirement | 回调前 / reader 注册前 / reader 启动前 / 主线程分配前 / 发布前 | 1 / 2 / 2 / 2 / 3 | 1 / 2 / 1 / 2 / 2 |
| synchronize_waits_for_old_reader | 注册前 / 主线程分配前 / 发布前 / synchronizer 启动前 | 1 / 1 / 2 / 2 | 各 1 |

合计 21 个场景。R1/R2 第九次更新前，读者已经发出启动通知并停在控制门上，已有 8 个旧值退休，source 还持有一个当前值；发布前注入则另有一份 unique_ptr 持有的新值。检查不只断言“没有挂死”，还逐场验证异常动态类型、原 point 和 what 字符串，确认没有被 broken_promise 或另一异常替换；检查准确的对象创建/销毁数与 worker 启动/完成数，并要求域正常析构，其析构检查读者表和 outstanding 为空。

每个返回的场景检查用时小于 5 秒；若真的挂死，内部时间检查无法运行，外部脚本的 30 秒进程超时负责判失败。进入每个场景前 flush 输出名称，便于从超时日志定位。原异常输出为 caught original: reclamation injected bad_alloc，结束输出为 21 injected failure paths ... PASS。

本次作者验证：MSVC /O2 /DNDEBUG /W4 /WX 的十个程序全部通过；相同十个程序的 ASan 构建也全部通过，21 个故障场景在两种构建下均通过，未报告地址访问错误。构建只使用 topics/reclamation/.build 和 .build-asan，没有使用主线程的 build/verify-core。

本节为 P2 返修时的历史验证记录；当时使用 verify.ps1 的普通和 ASan 模式。当前学生路径已经隔离，直接编译命令见第 1 节。若只复验既有协议的故障路径，在配置好编译器运行时路径的 Developer PowerShell 中执行：

```powershell
./C08_Concurrency/topics/reclamation/.build/exercises_runtime_tests_reclamation_test.exe --failure-paths
./C08_Concurrency/topics/reclamation/.build-asan/exercises_runtime_tests_reclamation_test.exe --failure-paths
```

未验证持续真实内存耗尽下的系统行为、线程创建器的实际故障、回收库内部再次资源失败、TSan 或其他平台。注入在一次指定位置抛异常，后续清理设施可工作；既有回收头的终止契约没有改变。测试有限结束与当前证据不能推广为任意调度、永久资源枯竭下的恢复保证。

### 本次文件与复验状态

修改 I3/R1/R2 的 reference.hpp 和 README、runtime_tests/reclamation_test.cpp、专题 03/04/05 正文、专题 README 与本验证记录；新增 topics/reclamation/experiment_support.hpp。共 13 个文件。未编辑 CMake、其他作者文件、三个公共回收头或 build/verify-core，未提交 git。没有新增链接依赖；原 runtime 目标直接重编译即可获得注入检查。

P2 修复现已由原独立 reviewer 复验，协议实现获 APPROVED（状态来自本次任务明确反馈）。本次新学生路径的复审状态另见第 7 节。下列 SHA-256 保留 P2 返修时五个变更文件的版本证据。

| 文件（相对 C08_Concurrency） | SHA-256 |
|---|---|
| exercises/I3_rcu/reference.hpp | `7a9836235a43bd7d0956a5be84079be637b2957266153613fd1b1b45755175c4` |
| exercises/R1_epoch_reclamation/reference.hpp | `5e1196fb031516f76c2e8dae7940886de973b629e2e9a2128d918d3b11b2832b` |
| exercises/R2_qsbr/reference.hpp | `d6f91fafee2ecc7d6f12b09ae97d54a0b3d5e53ce62aa3634b1f47313a918bf8` |
| exercises/runtime_tests/reclamation_test.cpp | `1ecb47c36b465cc13955a822cff1523ad55a0716406b68bdcdfac52743262737` |
| topics/reclamation/experiment_support.hpp | `199d31f935789307252a6e68e039ae34bbf1cc0f9f3d00c9e81a75164ec59b2f` |

## 7. 教学映射修复：独立学生路径与答案隔离

本轮阻断是作业路径和核对答案共用 reference.hpp，README 甚至要求学生重写它。现已为 I2/I3/R1/R2 各新增 student.hpp 与 checks.hpp，main 只包含 checks。学生只补 student.hpp 中的 TODO；solution/reference、公共 HP/EBR/RCU 算法和既有 runtime 检查都保持原样。公共回收库仍然复用，学生无需实现扫描器、登记表、宽限期或回调调度。

### Part 到实际代码和验收的对应

| 题目 | 学生操作 | checks.hpp 实际调用与观察 | 只读答案 |
|---|---|---|---|
| I2 A/B/C | protect；接收唯一所有权的 retire；pop 的 SC CAS；cleanup | 保护时删除数为 0，解除后真实删除；学生 pop 的 LIFO、空路径与 4000 个唯一 ID；清场必须在兜底析构前完成 | I2 的 solution.cpp/reference.hpp |
| I3 A/B/C | read(source,domain,use)；replace；checkpoint；shutdown | 在学生 read 的 use 内替换旧值并 collect，直接检查读区仍有效；四读者累计 8000 次；65 次析构和零 pending | I3 的 solution.cpp/reference.hpp |
| R1 A/B/C | pin；retire；collect；shutdown | 内层 pin 结束后外层仍保护；长读者时 64 份积压，退出后学生 collect 回收 64，最终 65 | R1 的 solution.cpp/reference.hpp |
| R2 A/B/C | online、read；quiescent；offline、shutdown | 读完但未报告仍保留 64；学生报告后在线时释放 64；participant 尚未析构就检查显式 offline 生效；最终 65 | R2 的 solution.cpp/reference.hpp |

学生路径不包含 reference.hpp，不调用任一 reference run。预检和并发阶段调用的是同一组 student 操作；不能通过只检查完成标志，再把实际工作转给答案来通过。输入生成、检查器、既有 signal/gate 和异常收尾由题目提供，学生方法的精确签名及 Part 范围在四题 README 中列明。

TODO 默认抛出带 Part 的 logic_error。main 先在单线程 preflight 实际调用每一步，包括 shutdown，然后才启动 worker；捕获未完成或结果检查异常后返回 1。没有完成标志，也不使用 77。预检同时查看源是否变化、是否真的登记退休、删除是否过早、回调是否已经结束；常量返回或空清场不会被视作完成。main 为故意抛异常导致的 MSVC C4702 单独加了局部诊断抑制，以便空 starter 在 /O2 /W4 /WX 下仍可编译，其他告警仍按错误处理。

预检不是对任意错误学生程序的安全证明。例如，学生无条件直接 delete 已退休对象仍可能造成未定义行为；这不属于空 TODO 或缺步骤的安全保证。题目提供的异常收尾仍遵循先放行全部门、再收束 worker、最后清理当前值和退休队列的顺序。成功条件在兜底清理之前检查，兜底不会替漏写的学生 shutdown 伪造通过。

### 独立编译和临时完成副本

为避免给学生目录留下已填答案，所有完成副本及缺步骤变体都通过 apply_patch 写入 .build/student-paths 私有目录。正式 student.hpp 保留全部 TODO。临时副本只填 student 操作，main/checks 及回收设施保持同样逻辑；分别编译为独立程序，不改动原 reference。

| 验证组 | 程序数 | Release | ASan |
|---|---:|---|---|
| 四个正式空 starter | 4 | 编译成功，运行均返回 1，未启动 worker | 相同 |
| 四个临时完成副本 | 4 | 实际学生检查 PASS，运行均返回 0 | 相同，未报告地址错误 |
| 每次只保留一个 TODO 未填 | 17 | 每个均在 preflight 返回 1，未启动 worker | 相同 |
| 已填版本故意把 cleanup/shutdown 改成空操作 | 4 | 真实结果检查拒绝，运行均返回 1，未启动 worker | 相同 |
| 原有四个 solution/Reference | 4 | 独立运行 PASS，返回 0 | 相同 |

共 33 个独立程序、两种构建，共 66 次运行。使用 MSVC C++23、/O2 /DNDEBUG /W4 /WX；ASan 加 /fsanitize=address /Z7。每个程序外部超时 15 秒。失败组除了退出码为 1，还检查输出中没有 starting student workers；成功学生组要求出现 student PASS。命名如 I2_hazard_pointer-completed、R2_qsbr-partial-5，stdout/stderr 保存在私有 release/asan 子目录。

另对本专题正式 verify.ps1 做模式验收：

- 默认 Reference：只运行四个 solution、reclamation_test 和 lifetime_reference，共 6 个程序全部 PASS；空 starter 不参与，不会阻断参考验证。
- Starter：四个原始空 main 均按预期返回 1，含 TODO 诊断且没有启动 worker；脚本确认“预期失败”后返回成功。这不是作业完成标记。
- Student 对原始空 main：脚本按预期失败，拒绝把退出码 1 当成完成。
- Student 对临时镜像中的四个完成副本：四个实际检查全部 PASS，脚本返回成功。

三种模式均独立选择源文件，没有公共 CMake/runner/索引改动；修改的是本专题自己的 verify.ps1，响应 Faraday 对“默认参考验证不能被空作业卡住”的反馈。未使用 build/verify-core，未提交或修改 git 状态配置。

### 不变性、改动清单和复审交接

修改前后比较了 13 个受保护文件的 SHA-256：三个公共回收头、四题 reference.hpp、四题 solution.cpp、既有 experiment_support.hpp 及 reclamation_test.cpp，全部完全一致。协议实现的 APPROVED 状态没有被本轮算法改动重新打开；本轮仍需检查的是新学生路径、检查覆盖和教学指引。

正式变更为四题各自的 main.cpp、README.md、新增 student.hpp、新增 checks.hpp，共 16 个文件；另同步专题 01/02 正文、专题 README、本验证记录和专题 verify.ps1，共 5 个文件。全部 21 个源码/说明/脚本文件的 SHA-256 见 [student-paths.sha256](student-paths.sha256)；该校验清单本身是第 22 个交付文件，其自身哈希在最终交接消息给出，避免自引用哈希循环。临时构建、完成副本及验证镜像不是学生提交内容。

四题 README 已明确“只编辑 student.hpp”；专题正文不再指引学生改写共享 reference。源码搜索也确认学生 main/student/checks 没有包含 reference.hpp 或调用其 run。后续交接分成两个结论：原 reviewer 对学生代码/检查器/异常收尾做技术复验；Faraday 对 Part → 学生待填操作 → 实际检查 → 只读答案 → 验证脚本模式做教学映射闭合。本轮作者自检通过，两个独立复验状态均待回传，不预先宣称闭合。
