# C08 教学与导航集成预审

结论：APPROVE 教学/导航结构。

本报告只审批当前 C08 的教学组织、跨课导航、覆盖去向、构建说明和已完成非作者复验的集成关系；不审批最终质量报告、最终矩阵统计或正式性能采样。`revision-quality-report-20260910.md` 仍是进行中占位，队列修订正式采样清单也明确等待独占窗口，不能据此宣称本轮 C08 全量最终通过。

## 实际读过范围与源指纹

| 范围 | 文件 | SHA256 |
| --- | --- | --- |
| 总纲 | `LEARNCPP_GLOBAL_PLAN.md` | `9E10066AE054E3B726FD46CEFB5ED9F60C8656ED8EB14779E27DECA60A5FCDAF` |
| 总纲 | `CONTENT_REFACTORING_GUIDE.md` | `971E2F48729FA198E41DAB09B8DC8B859745661AD3F184C8E061E2A11AD5E6FC` |
| C08导航 | `C08_Concurrency/README.md` | `29B8EFE536823F934579F7AF7F7021ACF19C95A545DC366728B609BCB6A5D469` |
| C08覆盖 | `C08_Concurrency/references/coverage.md` | `C4550C0B9A1A386489CAD307E342AB62226FEFF32D55AE71A1D019379868978D` |
| C08标准 | `C08_Concurrency/references/standards-and-implementations.md` | `6B59313CDEABE4DF29EC34EE618A9839112C14FDC4F7399F84697F743D39F467` |
| C08构建 | `C08_Concurrency/exercises/BUILD_GUIDE.md` | `71AB8BE3CCD696F91011FA4901FA0B7A43BFC0A7956753A21AC6924131D07E68` |
| 历史授权 | `C08_Concurrency/IMPLEMENTATION.md` | `FDEB9E96D7E22E96D36DBCAD0A77872E516FD107C26A67F53F3153A660EC696E` |
| 历史报告 | `C08_Concurrency/references/quality-report.md` | `AA3E77DE30E4B196B91399D6744796ACC1399662D058CE3633CDE376770B1252` |
| 本轮占位 | `C08_Concurrency/references/revision-quality-report-20260910.md` | `232FF47595DEB412A0D937FCEC46ACCF256B399C9CA66D118008BF2F80A03F0F` |
| 平台边界 | `C08_Concurrency/references/wsl-validation.md` | `9D215C98B6B804A37024A9BA56E38A6B85231D2BB61D1E47D8BD36E32B9C771A` |
| 队列证据 | `C08_Concurrency/topics/performance/c08-revision-queue-evidence.md` | `238AFD05350D2B7E2074FB0FFEBB1466211F784DDAB489E7D14910CF91077E63` |
| 旧性能记录 | `C08_Concurrency/topics/performance/VALIDATION.md` | `B89B3E2A6EAA906F7ED69955C7567372D5705A9A1142CB0480E623ECAF8C0A8B` |
| 日志桥接 | `C08_Concurrency/topics/logging/01-async-spdlog.md` | `0B6DE0A175F1CEA1734134B0FC6CF8133CD67C2609D7A0AD8D1B902BC552EB7A` |
| 日志练习 | `C08_Concurrency/exercises/U01_async_logging/README.md` | `BD4E5636E3A0C0733C0A15314F200E26AE26780DBC4B8E7662720A3EE6159F00` |
| C05桥接 | `C05_Data_Representation_Standard_Facilities/chapters/20-spdlog-frontend.md` | `307CBAA2BE32EC5EC1AD917B5A6B3E6CDF0FACD95D6BD5E8A2AF6D8A338FBB29` |
| L3 | `C08_Concurrency/exercises/L3_par_vs_seq_bench/README.md` | `1B87891B986434A8D81644E4843B88279301E92BA4102BFAD78DF064281C0181` |
| L3 | `C08_Concurrency/exercises/L3_par_vs_seq_bench/main.cpp` | `42FF245C863AF53A2DF0E0D66ADE699A7AF8FBE5C3FA02BDE1BDB90DEDF892A5` |
| L3 | `C08_Concurrency/exercises/L3_par_vs_seq_bench/checks.hpp` | `DD9C6030067A9C7C285790F31217695D0A18C7513812ED07589BCD4782BB8AE2` |
| L3 | `C08_Concurrency/exercises/L3_par_vs_seq_bench/reference.hpp` | `130CCE73E61AF2C2A7378F2DC2A3F1E2B146337C96327FB5B3C54DB48E525D7D` |
| L3 | `C08_Concurrency/exercises/L3_par_vs_seq_bench/solution.cpp` | `CE6A349836DBA54756F0883256DEF3D7BF8637896D98BA1854A3B1096BAE87B9` |
| L3 | `C08_Concurrency/exercises/L3_par_vs_seq_bench/benchmark.cpp` | `5CEF93A2FF7E41684BB3801642CFD0F52EE89BB6A35A4B6E3D0476E7072E0AEF` |
| L3 | `C08_Concurrency/exercises/L3_par_vs_seq_bench/CMakeLists.txt` | `3B0AE77C48774C01196DFA6B1F6B0B2BA8E50C844CDF635BE5A2C41C6542955F` |
| Cap3 | `C08_Concurrency/exercises/Capstone3_parallel_compute/README.md` | `820C2B10975B4FDD5C46EB8DA997D53C7585EAD28E70B741CA396212B7CC8BF2` |
| Cap3 | `C08_Concurrency/exercises/Capstone3_parallel_compute/main.cpp` | `5749012B439254939A3BD84B14AE6483BEB1C390E323463688FEC0C1CD93A259` |
| Cap3 | `C08_Concurrency/exercises/Capstone3_parallel_compute/checks.hpp` | `8CF4ED75412A60675855679BA93F5514DD6F6AFE9377A7B8616387372512FC8F` |
| Cap3 | `C08_Concurrency/exercises/Capstone3_parallel_compute/reference.hpp` | `C33064CD0DEAD1B8CD8E5512BC1CE3557FC3085C835633A994CA27E494E71B57` |
| Cap3 | `C08_Concurrency/exercises/Capstone3_parallel_compute/solution.cpp` | `E6C0F538685EF8BB234A72BE36C245DD8D9B38214BDBA1AA0E2A8B8D6BAEB508` |
| Cap3 | `C08_Concurrency/exercises/Capstone3_parallel_compute/benchmark.cpp` | `57902407FA5B1DDF0F5DAE31D0FBE6B0F03FE5B7596D322FC3C03814758023DE` |
| Cap3 | `C08_Concurrency/exercises/Capstone3_parallel_compute/CMakeLists.txt` | `81107DD169713B8EC4E19072E1AAF447DB6368C410A07D893C80488A8795ECA1` |
| M1/TSan | `C08_Concurrency/exercises/M1_work_stealing_pool/README.md` | `5597CFD2E94C1BEF974AE7591FB589AC5D6F1231FEB11A46CC20C1A18A8FB221` |
| M1/TSan | `C08_Concurrency/exercises/M1_work_stealing_pool/solution.cpp` | `3AC94D97F2BFF274FF311E16243566D72887BF59EB116BC0925B521961156535` |
| 已审证据 | `C08_Concurrency/references/validation/c08-revision/reviews/queue-sample-review-r2.md` | `DF2342581EBE41F414B2DB375337A39D40D51BB2E97F615A39795B9DC8492190` |
| 已审证据 | `C08_Concurrency/references/validation/c08-revision/reviews/logging-review-r2.md` | `C04B11425E014EB7D45B33B1EB0B4E592EE374DC82E20BEE11F4744AC567C7D0` |
| 已审证据 | `C08_Concurrency/references/validation/c08-revision/reviews/frontier-review-r5.md` | `1836C9F2F4F0104D53899DCB7ED1AFA0FC73F2ECE1C6D95A9FA384978ACC42F2` |
| 已审证据 | `C08_Concurrency/references/validation/c08-revision/reviews/tsan-fix-review.md` | `C65648A3FBBD33B04FE9EE3F4446507BCC89033F47D8D436F86FCE656AFD37C6` |
| 已审证据 | `C08_Concurrency/references/validation/c08-revision/reviews/tsan-m1-review.md` | `5705427827A32CD7EB97A03F7A23C2B9C76F63682C307EE7F3033253E357C424` |

补充计数口径：53 个普通练习按 `verify_materials` 的 `main.cpp` 加 leaf CMake 登记规则确认，即原 50 题 + U01 + F01 + F02。F03 是 3 个原生能力主体集合，另按 leaf CMake 注册的标准主体检查，不按普通 `main.cpp`/`solution.cpp` 作业计数。`C08_Concurrency/exercises` 还包含 `cmake`、`include`、`tools`、`runtime_tests`、`validation`、`build` 等非练习目录，不能用所有子目录数量做减法口径。

## 预审结论

1. 总纲约束已被当前导航承接。`LEARNCPP_GLOBAL_PLAN.md:52` 把 C08 定位为线程生命周期、同步、原子/内存序、线性化、并发结构、安全回收、调度和诊断；`README.md:21` 明确本课主讲共享状态、同步、发布和安全回收，CPU 性能/SIMD/NUMA 是 C13 可复用输入，不代表完整 C13 已交付。没有发现把 C13 完整性能课程提前宣称完成的导航问题。

2. 旧知识去向没有被新增主题覆盖掉。`references/coverage.md:78-84` 仍列出队列、回收、内存模型、NUMA/调度等原难主题；`references/coverage.md:93-99` 用反向问题串起等待/关闭、发布、队列、回收、性能、NUMA/调度。`references/coverage.md:107-111` 明确旧 L3、J1/J2、NUMA 等去向，并说明删除的是重复清单和无依据保证，不是删除实质知识。

3. 新增 U01/F01/F02/F03 接线清楚。`references/coverage.md:11-15` 分别登记 C05 同步日志到 U01、线程属性到 F01、HP batch 到 F02、标准 HP/RCU/sender 原生主体到 F03、队列演进证据到性能记录；`README.md:67-73` 提供读者入口。F03 被表述为集合，且 `BUILD_GUIDE.md:119` 说明 F01/F02 是 C++29 单元，F03 三个标准主体分别构建运行；不会因 F03 没有普通作业入口而误报缺练习。

4. C05 第20章只做同步前端，不抢 C08/C11。`C05.../20-spdlog-frontend.md:3-5` 明确 C05 只讲同步 logger 前端、sink、pattern、格式化错误处理，异步队列、溢出策略、flush/shutdown 和服务观测分别留给 C08/C11。`topics/logging/01-async-spdlog.md:3-15` 承接 C05 并把 C08 范围限定为本地进程内队列、worker、错误和生命周期；`U01_async_logging/README.md:3-9` 给出先修链和固定 spdlog/fmt 输入。桥接方向正确。

5. L3/Cap3 检查头分离后仍可遮答案完成。L3 README `:5-17` 和 Cap3 README `:3-21` 均说明 `main.cpp` 是 IMPLEMENTATION Starter、`solution.cpp` 是独立 Reference、`benchmark.cpp` 是提供驱动。L3 `main.cpp:7-18` 只暴露 `student_light/student_heavy/student_map` 待实现路径，`checks.hpp:8-17` 是独立校验；Cap3 `main.cpp:7-17` 暴露 `student_gemm/student_sum/student_sort`，`checks.hpp:14-22` 是独立矩阵 oracle。`solution.cpp` 均只 include `reference.hpp`，未被学生入口调用。当前结构满足“Reference 通过不能冒充学生完成”的要求。

6. L3/Cap3 的性能叙事没有越过证据边界。L3 README `:70` 和 `topics/performance/VALIDATION.md:70-72` 明确旧开发样本不是当前冻结版本正式性能结论；Cap3 README `:59-86` 给出实验任务和 runner 流程，`VALIDATION.md:133-144` 保留历史元数据并要求后续正式采样使用 `_benchmark` 目标。该口径符合 `CONTENT_REFACTORING_GUIDE.md:185-195` 的“先定位、分开正式计时、保存原始样本、不同契约不混排”。

7. 队列 r2 的定位证据与正式采样边界被分开。`topics/performance/c08-revision-queue-evidence.md:3` 明确该记录不新增正式 benchmark 结论；`:31-35` 说明诊断目标统计真实 transfer 路径、锁进入、SPSC 远端下标读取和单线程热区分配；`:48-50` 区分公开调用/内部 mutex 和诊断扰动；`:62-71` 保留正式采样清单等待独占窗口。此前 r2 非作者审查已批准诊断逻辑，不批准正式数据结果；当前导航没有把诊断 CSV 写成最终性能结论。

8. TSan/M1 边界没有把检测器局限教成语言规则。`BUILD_GUIDE.md:164` 明确 ASan/TSan 都不是内存序或进展保证证明；`chapters/17-diagnostics-and-sources.md:89-95` 区分 ASan/TSan 能力和未报告边界；`wsl-validation.md:37-41` 把控制通过、已知竞争检出、工具组合限制和 SKIP/PARTIAL_SKIP 分开。M1 README `:9-11` 将 Clang18/libstdc++13/TSan 的异常消息文本问题限定为工具链边界，保留标准异常寿命结论；M1 solution `:8-13`、`:88-91` 只在精确组合下 PARTIAL_SKIP，不加 suppression 或额外 join。该叙事没有用 TSan 结果改写 C++ 语言规则。

9. 构建指南能支撑当前教学分类。`BUILD_GUIDE.md:66-84` 明确 Student、Observation、Reference 的 CTest 名称与含义，77 只表示明确能力缺失，不表示作业没写；`:115-129` 区分 C++29、C++26、第三方和标准能力探测；`:198-211` 说明 U01 默认关闭、固定源码、队列诊断 target 与正式 bench target 分离。没有发现“空目录/测试数量/Reference 通过”被写成课程完成证据。

10. 历史授权边界清楚。`IMPLEMENTATION.md:3` 明确 2026-09-08 历史授权不赋予后续提交、推送或关机权限，本轮以 `revision-plan-20260910.md` 和 `revision-quality-report-20260910.md` 为准；`:70-74` 的旧最终状态保留原义。当前材料没有把历史质量报告扩张为本轮新 U01/F01/F02/F03 或队列修订的最终批准。

## 待最终审查项

这些不是本预审的退回项，但必须保留为后续最终质量审查范围：

- `revision-quality-report-20260910.md:19-23` 仍记录新增日志、前沿、学生资产、材料导航、演进定位、正式采样、最终集成审查尚需闭合；当前不应把该报告当最终通过报告。
- `topics/performance/c08-revision-queue-evidence.md:62-71` 的正式队列采样尚未开始；不能用 r2 诊断或旧 `final-20260908` 样本替代当前冻结源码的正式采样。
- F03 当前能力缺失下的 SKIP 只说明标准库接口不可用；未来 `CS_HAS_* = 1` 后仍需以主体编译/运行结果判 PASS/FAIL。
- 53 普通练习目录与 F03 原生集合是两个计数口径；最终报告应继续避免把 F03 空 `main.cpp`/`solution.cpp` 或测试数量变化写成独立完成证明。

## 本轮验证动作

- `rg --files` 定位 C08/C05/总纲/评审范围。
- `Get-FileHash -Algorithm SHA256` 绑定上述读过文件。
- `Select-String` 按 U01/F01/F02/F03、L3/Cap3、TSan/M1、C05/C09/C10/C13、正式采样、Student/Reference/Observation 等关键字核对事实行。
- 计数说明更正为 `verify_materials` 的 `main.cpp` 加 leaf CMake 登记规则：53 个普通练习 = 原 50 题 + U01 + F01 + F02；F03 是另列原生主体集合，`exercises` 下其他基础设施目录不参与普通练习计数。

未运行构建或 CTest；本任务是只读教学/导航集成预审，且前序日志、队列、前沿、TSan 专项已有独立复验报告。本报告没有修改被审内容。

