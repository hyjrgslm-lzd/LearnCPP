# QSBR：独立学生上线、报告与下线

完整推导、时序与来源见 [专题正文](../../topics/reclamation/04-qsbr.md)。

## 本题任务与验收

| Part | 只在 student.hpp 补全 | 检查器实际检查 |
|---|---|---|
| A1 | online(domain)：返回已在线 participant | 在线旧代确实阻止回收 |
| A2 | read(source)：复制 int 载荷 | 使用 SC 源，返回复制的 42，不返回裸借用 |
| B | quiescent(participant) | 报告后 worker 仍在线时，旧代 64 份实际可回收 |
| C1 | offline(participant) | participant 尚未析构时，新退休记录已不再被它阻挡 |
| C2 | shutdown(source, domain) | join 后清源、退休最终值并完成回调 |

写者发布/退休、预算、collect 观察、控制门闩和异常收尾已给出，本题重点是应用什么时候正确报告静默。online 返回 prvalue，不要求修改公共 participant 的移动限制。read 返回独立 int 值，报告前必须结束所有旧借用；不能把原指针留给下一轮。

预检先观察在线旧代阻挡回收，再调用学生 quiescent 解除旧代阻挡；接着在 participant 仍存活时调用学生 offline 并追加退休，确认不是靠析构碰巧下线。并发实验把“读完”“报告”“离开”分开：报告前积压 64，报告后线程仍在线时回收 64，最终析构总数 65。

自测答案：读完不等于回收器知道已读完；sleep/yield 不会报告静默；quiescent 后不能再用旧裸借用；offline 必须先于需要下线的长期阻塞。见正文第 1—6 节。请只补 student.hpp；reference.hpp、epoch.hpp 和检查器保持只读。

## 编辑边界与运行方式

这是独立的学生补全题。只编辑本题 `student.hpp` 的 TODO；`main.cpp → checks.hpp → student.hpp` 是作业路径，`solution.cpp → reference.hpp` 是只读答案路径。检查器不会调用 reference::run，也不让学生改 reference.hpp、检查器、公共回收头或完成标志。没有“完成=true”开关。

每个 TODO 默认抛出带位置的 logic_error。main 先执行单线程 preflight，实际检查每个学生操作的效果；有任一步未填或预检失败，在线程启动前输出 student FAIL 并返回 **1**。不是 77。只有预检通过才出现 `preflight PASS; starting student workers`，并启动并发检查。删除异常、返回常量或把操作改为空函数，不能替代对应的状态变化与回调结果。

在仓库根目录的 x64 MSVC Developer PowerShell 编译本题 main（把下方题目 ID 代入路径）：

```powershell
New-Item -ItemType Directory -Force Concurrency_Study/topics/reclamation/.build | Out-Null
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG /IConcurrency_Study/exercises/include /FoConcurrency_Study/topics/reclamation/.build/R2_qsbr_student.obj /FeConcurrency_Study/topics/reclamation/.build/R2_qsbr_student.exe Concurrency_Study/exercises/R2_qsbr/main.cpp
./Concurrency_Study/topics/reclamation/.build/R2_qsbr_student.exe
$LASTEXITCODE
```

空 starter 的编译应成功、运行退出码应为 1；补全后全部检查通过才返回 0。核对答案时独立编译本题 solution.cpp，另用 `R2_qsbr_reference` 作为输出名；它不包含 student.hpp，学生修改不会改变答案。

专题验证脚本已区分三种模式：`verify.ps1`（默认 Reference）只运行四题答案、runtime 和生命期 reference，不编译未完成 main；`verify.ps1 -Mode Starter` 检查四个空作业均在启动线程前按预期返回 1；`verify.ps1 -Mode Student` 检查四个已补全作业实际通过并返回 0。各模式可加 `-Asan`，脚本成功表示符合该模式的预期，不能把 Starter 的脚本成功理解为作业完成。直接编译与本次复审证据见 [验证记录](../../topics/reclamation/06-validation.md)。

检查使用 cs::check，Release 下仍执行。MSVC 的学生入口仅抑制 C4702：故意抛异常的 TODO 在 /O2 下会让编译器识别出成功分支不可达；其他 /W4 /WX 检查保留。这个诊断设置不决定通过结果。

## 共用设施与责任

默认 C++23，源指针全部使用 SC。回收库、读区协议、门闩异常传播及输入生成已提供，不要求重写公共算法。用户操作仍须遵守正文的同域、不可变载荷、一次退休和保护边界约束；完整操作不承诺 lock-free。空 TODO 的预检是安全入口约束，不是对任意错误学生代码的内存安全保证。错误实现应另用 ASan 和外部超时诊断。

依赖：C++23 标准库、线程支持及 exercises/include；I3/R1/R2 的检查器复用 topics/reclamation/experiment_support.hpp。没有新链接库。main 与 solution 必须分别编译为独立程序，不能链接两个 main。
