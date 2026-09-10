# 队列专题：作者验证与集成交接

本记录包含 2026-09-08 的初稿作者验证和随后独立 review 发现 P1/P2 后的修复验证。当前修复已完成作者重测，等待原非作者复验，不能标为独立通过。回收作者的 [最终接口与兼容性记录](../reclamation/06-validation.md)也不是本队列协议的完整审查。第 2、3 节保留修复前记录；第 5 节为当前源码哈希，第 6 节给出本次修复、赋值审计和重测结果。

## 1. 已完成的变更文件

所有主动编辑均在 C08_Concurrency 内。未修改任何 CMake、根 README、公共 benchmark 工具、标准/覆盖/质量索引、queue_baseline.hpp 或已审批的 01-mutex-baseline.md；未提交 git。共享工作树中其他作者的 CMake 和回收头修改保留原样。

| 范围 | 本作者文件 |
|---|---|
| 学习入口 | chapters/11-concurrent-structures.md |
| 连续正文 | topics/queues/02-bounded-and-batch.md、03-spsc.md、04-mpsc.md、05-vyukov-mpmc.md、06-michael-scott.md、07-treiber-and-aba.md、08-validation-and-benchmark.md、本记录 |
| 队列专项头 | exercises/include/concurrency_study/queue_versions.hpp、queue_linked.hpp、queue_checks.hpp |
| 旧题同步 | G1_treiber_stack、G2_aba_problem、G3_spsc_ringbuffer、Capstone2_lockfree_queue 各自的 main.cpp、solution.cpp、README.md；G2 另有 reference.hpp |
| 新题 | Q1_mpsc_queue、Q2_ms_queue 各自的 main.cpp、solution.cpp、README.md |
| 正确性与计时 | exercises/runtime_tests/queue_history_test.cpp、exercises/benchmarks/queue_bench.cpp |
| 旧入口迁移 | 09-模块G-无锁数据结构.md、12-第二阶段结课-无锁队列.md |

按 ponytail 的复用原则，MPSC 是同一 sequence_ring 的单消费者编译期分支，SPSC 缓存版也是同一类型的分支；正确性驱动与基准共享这些实现。这里没有把减少重复代码当作省略正文、保护协议或可运行检查的理由。

## 2. 修复前作者验证结果（历史记录）

环境是 Windows 11 build 26220、x64、MSVC 19.51.36256（工具目录 14.51.36231）、Windows SDK 10.0.26100.0。直接编译使用 /std:c++23preview /EHsc /utf-8 /O2 /DNDEBUG /W4；ASan 再加 /fsanitize=address /Z7，链接启用 /DEBUG 并使用同版本 ASan DLL。常规最终编译没有警告。

- 六题的 main 与六个 Reference 均直接编译、运行成功。
- G1、G2、G3、Capstone2 的现有叶 CMake 已完成 Release 构建及 CTest，各自 Reference 通过；runtime_queue_history_test 也完成公共 CMake 构建和 CTest，共五项 CTest 通过。queue_bench 的现有 benchmark CMake 目标 Release 构建成功。CTest 使用公共配置的 30 秒上限。
- G1、Q2、queue_history 的 ASan Release 均运行成功，标准输出包含正常完成信息，捕获 stderr 未报告地址访问错误。
- queue_history 每次执行 20 组、每组 8 种结构/分支的实际小历史；另有检查器正反例、真实预订暂停、8 位循环计数模型。不是无限状态穷举。
- Q2 的确定性暂停检查验证 next 保护延迟实际析构，恢复后 cleanup 确实释放；G1/Q2 的非平凡元素计数都验证运行中清理和最终归零。
- benchmark 的 13 个非法参数场景均返回 1，stdout 为空、stderr 带 queue_bench 诊断；覆盖 all、容量 0/1/3、非法角色、MS 非零容量、非 batch 版本的批量参数、零工作量/角色/批量、超出角色预算与未知选项。
- 课程内 Markdown 文件链接检查零失效。所有代码链接指向实际文件；Q1/Q2 尚无作者自行创建的 CMake，README 同时提供已初始化 Developer PowerShell 的直编命令。

沙箱内初次 CMake 失败先暴露重复 PATH/Path，再在规范化子进程环境后暴露 MSBuild FileTracker 的 E_ACCESSDENIED。经自动批准在沙箱外运行相同叶项目构建后，以上 CMake/CTest 已完成；没有修改构建源文件来绕过错误。作者产物在 exercises/build/queue-author-* 目录。

未运行 GCC/Clang、TSan、LSan、ARM/弱内存硬件、形式化模型检查器。ASan 不检测所有数据竞争，也不是完整泄漏证明；live 计数只覆盖测试类型及本实现释放路径。未测暂停线程的任意长寿命、内存耗尽死亡路径、HP 删除器终止路径、公平性、尾延迟或真实堆内存峰值。C++26 原生 HP 未测试，当前使用 C++23 cs 教学域。

## 3. 修复前的40个外部样本（历史快照）

本节全部数值来自修复前实现，不是当前 T[]/const 源复制实现的测量结果。旧输入为 size_t，初次测试没有覆盖后来 review 发现的 bool 压位与双重载类型问题。本次修复只对新 benchmark 做重建和八版本接口冒烟，没有重跑正式五次采样；当前版本没有新的吞吐结论。保留原样本及文件用于追溯，不把旧结果重新标成新实现。

先做了一轮全部 variant 的 runner 接口冒烟，目录是 build/queue-author-results-*。这些组曾同时运行，只用于 CSV/完成量兼容性检查，不拿其耗时做性能对照。

随后五组逐组顺序执行，目录改为 build/queue-author-samples-*。每个版本暖机一次、正式五次独立进程、种子 42；每轮 size=100003，completed 全部精确为 100003。每组开始前运行 history 与对应 Reference，runner 最终均为 PASS；各组没有由本作者同时启动其他基准。未控制整台机器的其他后台负载、核心亲和性、功耗状态或 NUMA。

runner 环境记录 AMD64 Family 25 Model 97 Stepping 2、AuthenticAMD、32 个逻辑 CPU；WMI 查询具体 CPU 型号因权限失败，所以不补写一个猜测型号。旧 benchmark SHA256 为 `4f294f94418659ba63c569c7a12cdfda24951bce9e0a0c0fbb97f50c0aaeb524`。对应旧 queue_versions.hpp 为 `55d2794b94411cc28dbbe8de127c8f8a5dcb85bb564ab1990dd36ac8eaf315fe`，旧 queue_linked.hpp 为 `23b798b24363a89be672d63800cfaf65ded520166bf145c8ee2e96e158e97e22`，旧 queue_checks.hpp 为 `2adff57c12142394052044a2b00deb2c1c10fbb759132b13681e9d5b5b233cc5`。runner 的 run.json 还保存当时的源目录摘要、Reference 二进制摘要、每轮命令、stdout/stderr 和耗时。这些摘要不能替换为第 5 节修复后源码哈希。

单位均为毫秒；五个正式样本按 runner 记录顺序完整列出，未删除离群点。下表按契约/拓扑分组，不构成跨组排名。

| 组、variant | P/C、容量、batch | 五个正式样本 | 中位数 | 范围 |
|---|---|---|---:|---|
| storage / mutex | 3/4、64、1 | 12.7760, 18.6527, 11.1673, 16.8075, 12.4406 | 12.7760 | 11.1673–18.6527 |
| storage / ring | 3/4、64、1 | 20.3887, 13.2970, 22.1876, 15.4127, 21.6831 | 20.3887 | 13.2970–22.1876 |
| spsc / spsc | 1/1、64、1 | 1.5334, 1.2953, 1.0756, 1.3196, 1.1966 | 1.2953 | 1.0756–1.5334 |
| spsc / spsc-cached | 1/1、64、1 | 1.1305, 1.3491, 1.0818, 1.1803, 1.1309 | 1.1309 | 1.0818–1.3491 |
| mpsc / mpsc | 3/1、64、1 | 7.6070, 6.4272, 5.9977, 7.8447, 7.5738 | 7.5738 | 5.9977–7.8447 |
| mpsc / mpmc | 3/1、64、1 | 7.6957, 8.6459, 7.5097, 7.8495, 11.3129 | 7.8495 | 7.5097–11.3129 |
| batch / batch | 3/4、64、8 | 6.7833, 8.3842, 4.2763, 6.4905, 8.5948 | 6.7833 | 4.2763–8.5948 |
| ms / ms | 3/4、无界、1 | 55.5749, 42.9428, 45.0709, 36.3218, 41.9066 | 42.9428 | 36.3218–55.5749 |

这些五组最终样本已复制进仓库的 `references/measurements/final-20260908/queue-storage`、`queue-spsc`、`queue-mpsc`、`queue-batch`、`queue-ms`，每组都有 run.json 与 samples.csv。早期作者 build 目录仍只是临时归档；仓库最终样本才是可回读证据。它们绑定旧 benchmark 与旧算法头 hash，不能自动代表第 5、6 节修复后的当前源码性能。

这次预分配 ring 的中位数高于 mutex，说明“消除容器分配必然加速”没有得到支持。SPSC 缓存版中位数较低，但样本范围重叠，不能推成普遍保证。MPSC 裁剪在此输入下差距也小。batch 改变调用粒度且另组执行，MS 又改变容量与分配/回收，不能根据这两行与其他组直接推算同契约加速比。热点原因、最大延迟与更广工作量仍未验证。

## 4. 主线程集成信息

主线程已为 Q1_mpsc_queue、Q2_ms_queue 接入常规叶 CMake：main.cpp 与 solution.cpp 分别注册，单个可执行文件各一个源。本次已用它们构建并执行 Reference。无定制多源、无额外库，使用公共 include、线程支持及 C++23。G2/reference.hpp 和三个专项头不单独编译。

runtime_tests/queue_history_test.cpp 与 benchmarks/queue_bench.cpp 都是单源入口，现有 glob 可自动发现；只构建相应目标即可。四个旧题的 solution.cpp 可由 cs_add_exercise 自动注册。

HP 需求已满足：MS enqueue 一槽、dequeue 两槽，Treiber pop 一槽；调用结束即释放局部句柄。源原子全 SC，没有 raw reset_protection。只使用最终头现有 cleanup，不要求第三个槽、新 overload 或公共头修改。最终头的完整边界参见回收作者记录；cleanup 返回 0 没有被当作全局无退休证明。

benchmark 具体 variant 是 mutex/ring/batch/spsc/spsc-cached/mpsc/mpmc/ms。参数是 --size、--producers、--consumers、--capacity、--batch；SPSC 要 1/1，MPSC 要 C=1，MS 要 capacity=0，只有 batch 允许 batch>1。stdout 只输出 CSV，名称与 --variant 一致，未知参数拒绝。SPMC 在 Capstone2 的 mpmc 与 Q2 的 ms 运行检查中已覆盖；这不是专用 SPMC 性能最优结论。

待主线程：安排原非作者针对 P1/P2 修复版复验，并按新的头文件版本处理总集成。本次没有访问或改动 build/verify-core，也没有改任何共享 CMake。

## 5. 修复后核心文件版本（SHA256）

共享 hazard_pointer.hpp 与已审批 queue_baseline.hpp 列在这里仅用于绑定依赖版本，本次未编辑。下表为本次重建采用的当前版本；不表示原非作者复验已经通过，也不绑定第 3 节历史样本。

| 文件 | SHA256 |
|---|---|
| [exercises/include/concurrency_study/queue_versions.hpp](../../exercises/include/concurrency_study/queue_versions.hpp) | `932796d46ab53a59347f2c5fff0cc69d7b8ac9c8bca2daf375846adf72623513` |
| [exercises/include/concurrency_study/queue_linked.hpp](../../exercises/include/concurrency_study/queue_linked.hpp) | `4f5013a93051a889f5c120eb8358f9b725fff835be495ebe24ae66416429cebf` |
| [exercises/include/concurrency_study/queue_checks.hpp](../../exercises/include/concurrency_study/queue_checks.hpp) | `17f3c87cfc0fb741b76c861330145792a63ff4621f3bc0a8c3b29e9329d1ec69` |
| [exercises/include/concurrency_study/hazard_pointer.hpp](../../exercises/include/concurrency_study/hazard_pointer.hpp) | `4c2ad89d79df051d1b4d1c79be9a1624c5dc119cbdaee9deb776bd1f36a2a955` |
| [exercises/include/concurrency_study/queue_baseline.hpp](../../exercises/include/concurrency_study/queue_baseline.hpp) | `1fc70f9529dc7ac858672fe17713862d65b936a09e652807a584730776a4af9d` |
| [exercises/runtime_tests/queue_history_test.cpp](../../exercises/runtime_tests/queue_history_test.cpp) | `8eb80ab7e6804cd774c10070ae4c4d14033b82fe5c8c4fb1cb460927cda37d0f` |
| [exercises/benchmarks/queue_bench.cpp](../../exercises/benchmarks/queue_bench.cpp) | `ee23b48bf237b29ee2087c2159c35418ca04179e3687e0a08dd6999c405c4a71` |
| [exercises/G1_treiber_stack/solution.cpp](../../exercises/G1_treiber_stack/solution.cpp) | `1b0a6337d538b38837589d4ccac1e53f59fd8ed3cea837604fefee51f9cf7013` |
| [exercises/G2_aba_problem/reference.hpp](../../exercises/G2_aba_problem/reference.hpp) | `a01335b1ddfee64d0cf81b26b313dbfcc75a2f2c4cdc38d3661733312da699c9` |
| [exercises/G2_aba_problem/solution.cpp](../../exercises/G2_aba_problem/solution.cpp) | `466158dbe569643b8a9278b5f617abc42c95c43ff9c2c8e8bb83ba25bd7fc8e1` |
| [exercises/G3_spsc_ringbuffer/solution.cpp](../../exercises/G3_spsc_ringbuffer/solution.cpp) | `367f734bd118315aa12a02ea014aa137f332ebd037a32290ef4542f023820107` |
| [exercises/Capstone2_lockfree_queue/solution.cpp](../../exercises/Capstone2_lockfree_queue/solution.cpp) | `30df5ade5b86b71a2657b975d11fc4c78bd00d9b07a06a802703255941803c03` |
| [exercises/Q1_mpsc_queue/solution.cpp](../../exercises/Q1_mpsc_queue/solution.cpp) | `2024f0aa4956544d07bfa6c190ae41358eb7e1cae2502147eb0782758d234e09` |
| [exercises/Q2_ms_queue/solution.cpp](../../exercises/Q2_ms_queue/solution.cpp) | `2c4415264603d3f57edc8bfdc204195723d833b804b3622a4b2653b8ded13a0c` |

## 6. 独立 review P1/P2 修复与作者重测

P1 的根因是把逻辑槽位独占推导套到了 vector<bool> 的压位表示。SPSC 普通与缓存版现在共用 `unique_ptr<T[]>` 和实际槽数，构造用 make_unique<T[]>，bool 仍然合法；两个实际 bool 对象不再通过代理共享一个可变位字。mutex_ring 的 vector<bool> 仍受同一把 mutex 保护，不存在这条无锁访问缺口，故保留原存储。

P2 的根因是 trait 与真实源表达式不一致。全课程 grep 找到的队列相关调用已经沿入队、出队、批量与节点构造核对，结果如下。这里只修改本专题输出复制，没有修改其他作者客户端或放宽 noexcept 约束。

| 路径 | 实际源类别与处理 | 回归入口 |
|---|---|---|
| mutex_ring push_batch / 单项 push | span<const T> 元素，本来就是 const T& | Capstone2 |
| mutex_ring pop_batch / 单项 pop | 改为 as_const(data_)[head]；一般为 const T&，vector<bool> const 访问为 bool 值 | Capstone2 双重载及 bool 前缀 |
| SPSC 普通/缓存 push | 参数 const T&，无需转换 | G3 |
| SPSC 普通/缓存 pop | 改为 as_const(data_[head]) | G3 两版双重载 |
| MPSC/MPMC push | 参数 const T&，槽本来就是独立 T | Q1 / Capstone2 |
| MPSC/MPMC pop | 改为 as_const(slot->data) | Q1 / Capstone2 双重载 |
| Treiber 节点构造 / pop | 构造参数 const T&；pop 改为 as_const(old->value)，防止成功摘除后跳过 retire | G1 双重载及寿命计数 |
| MS 节点构造 / pop | 构造参数 const T&；const optional 解引用已为 const T&，只补说明注释 | Q2 双重载及既有暂停回收 |
| 已审批 mutex_queue | 输出使用 std::move，匹配 is_nothrow_move_assignable；不改基线头 | Capstone2 同一探针兼容检查 |
| I2 HP 栈示例 | 输出源为固定 int，非泛型重载；不改回收作者文件 | 只读核对，不宣称本次重跑 I2 |

copy_overload_probe 通过静态检查确认 const 赋值确实 noexcept、可变源赋值确实非 noexcept；后者的函数体真实抛异常。八种队列配置实际执行单项复制及存储复用，适用时执行批量部分接受和未填后缀检查，要求可变源重载调用次数为零、最终 live 为零。所有常规调用都应成功，未将故意终止路径设为正常完成。

G3 的 bool 回归同时覆盖普通与缓存版，容量 1/2/7/64，每配置先做容量边界，再传输 20003 个非恒定真/假值并逐项核对。没有用 vector<bool> 记录观测；producer 只执行 noexcept 操作，主线程检查失败会取消并 join。ASan 不是数据竞争检测器，本轮未运行 TSan，因此安全性依据是实际数组对象及槽位发布/归还协议，而非“没有报告错误”。

本轮具体结果：

- G1、G3、Capstone2、Q1、Q2：MSVC Release 直接编译使用 /W4 /WX，全部无警告、运行成功。五个对应叶项目另行完成 Release CMake 构建，五个 Reference CTest 均通过；历史检查重建后 CTest 通过，共六项。原历史搜索算法未修改。
- 上述五个 Reference 加 queue_history：全部以 /O2 /DNDEBUG /W4 /WX /fsanitize=address /Z7 重建，六个程序在各自 30 秒外部上限内成功结束，捕获 stderr 无 ASan 报告；bool 与双重载新回归均实际运行。
- 当前 queue_bench 已通过 CMake Release 重建。八个具体 variant 各独立运行一次、size=10003；逐行验证标准 CSV 表头、单行结果、variant 名、线程总数、completed=10003 和空 stderr，全部通过。此为接口冒烟，耗时未用作正式样本或排名。

所有本轮产物使用 `exercises/build/queue-review-fix-*`，没有使用主线程的 build/verify-core，也没有覆盖修复前样本目录。CTest 证据在各叶构建目录的 Testing/Temporary/LastTest.log；ASan 可执行文件在 build/queue-review-fix-direct，后缀为 -asan.exe。当前 CMake benchmark 的二进制 SHA256 是 `b83330d80af2f50c214122c1569858a74da104a6164f51a11f5e5f6938a4eca9`，与第 3 节旧采样二进制不同。

复现本轮已构建的检查，可从 exercises 执行（每个目录只含本专题目标或使用指定过滤）：

```powershell
foreach ($id in @('G1_treiber_stack','G3_spsc_ringbuffer','Capstone2_lockfree_queue','Q1_mpsc_queue','Q2_ms_queue')) {
    cmake --build "build/queue-review-fix-$id" --config Release
    ctest --test-dir "build/queue-review-fix-$id" -C Release --output-on-failure
}
cmake --build build/queue-review-fix-runtime_tests --config Release --target runtime_queue_history_test
ctest --test-dir build/queue-review-fix-runtime_tests -C Release -R '^runtime_queue_history_test$' --output-on-failure
```

本轮源修改清单：queue_versions.hpp、queue_linked.hpp、queue_checks.hpp；G1/G3/Capstone2/Q1/Q2 各自的 solution.cpp 与 README.md；队列02–08正文及本记录。公共 CMake、HP 头、基线头、历史搜索源码、benchmark driver 源码均未修改。待原非作者基于第 5 节版本复验 P1/P2；当前状态是修复后作者验证通过，非独立通过。

## 7. 2026-09-10 队列样章证据口径补充

本轮只补队列 01-03 的演进证据口径、最终样本回链和诊断入口，不做正式 benchmark。新增 [`queue_diagnostics.cpp`](../../exercises/benchmarks/queue_diagnostics.cpp) 在 `CS_QUEUE_DIAGNOSTICS` 开启时统计公开接口调用、成功调用、完成元素数、mutex 进入、SPSC 远端下标 load，并在隔离单线程热区统计 `operator new`。它可用于样章自检，不能证明锁等待、CAS 失败、cache miss 或内核调度原因。

新说明见[队列修订证据口径](../performance/c08-revision-queue-evidence.md)，r2 自检输出为 `references/measurements/c08-revision-queue-evidence/queue_diagnostics-r2.csv`。正式样本必须等主线程测量窗口后运行：一轮 warmup、五次独立进程、seed=42、保留所有原始样本及负收益。
