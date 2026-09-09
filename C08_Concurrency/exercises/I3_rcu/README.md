# RCU：独立学生读区、替换与清场

完整推导、时序与来源见 [专题正文](../../topics/reclamation/05-rcu.md)。

## 本题任务与验收

| Part | 只在 student.hpp 补全 | 检查器实际检查 |
|---|---|---|
| A | read(source, domain, use)：先创建读区，再 load 并 use(*p) | use 确实执行，字段自洽；在 use 内退休旧值时仍不能回收 |
| B1 | replace(source, unique_ptr<config>, domain) | 源已换成新版本，旧版本进入同域退休队列 |
| B2 | checkpoint(domain) | 已退休回调确实结束；只有 synchronize 或空操作不能通过 |
| C | shutdown(source, domain) | 在读者 join 后清源、退休最终值、等待回调完成 |

config 的构造、不变量、统计与线程组织已提供。use 是只在读区内执行的借用回调，不允许保存 config 的裸指针/引用供读区外使用。replace 接收尚未发布的新版本所有权，SC exchange 之后再把旧值退休到同一个域；多写者增量更新不在本题范围。

预检在学生 read 的 use 回调里调用学生 replace，并尝试 collect；这样会直接暴露“读区没覆盖 use”的错误。随后检查 checkpoint 和 shutdown 的真实析构结果。并发阶段四个读者各读取 2000 次，写者替换 64 次并每 16 次调用学生 checkpoint，最后检查 8000 次读取、65 次析构和零 pending。

自测答案：生命期保护不允许原地并发改普通字段；synchronize 仅等待宽限期，checkpoint/shutdown 需要回调完成；join 只收束读者，不排空回收队列。见正文第 1、4—7 节。Reference 及其既有故障注入检查保持只读，不要求学生重写那套实验调度。

## 编辑边界与运行方式

这是独立的学生补全题。只编辑本题 `student.hpp` 的 TODO；`main.cpp → checks.hpp → student.hpp` 是作业路径，`solution.cpp → reference.hpp` 是只读答案路径。检查器不会调用 reference::run，也不让学生改 reference.hpp、检查器、公共回收头或完成标志。没有“完成=true”开关。

每个 TODO 默认抛出带位置的 logic_error。main 先执行单线程 preflight，实际检查每个学生操作的效果；有任一步未填或预检失败，在线程启动前输出 student FAIL 并返回 **1**。不是 77。只有预检通过才出现 `preflight PASS; starting student workers`，并启动并发检查。删除异常、返回常量或把操作改为空函数，不能替代对应的状态变化与回调结果。

在仓库根目录的 x64 MSVC Developer PowerShell 编译本题 main（把下方题目 ID 代入路径）：

```powershell
New-Item -ItemType Directory -Force C08_Concurrency/topics/reclamation/.build | Out-Null
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG /IC08_Concurrency/exercises/include /FoC08_Concurrency/topics/reclamation/.build/I3_rcu_student.obj /FeC08_Concurrency/topics/reclamation/.build/I3_rcu_student.exe C08_Concurrency/exercises/I3_rcu/main.cpp
./C08_Concurrency/topics/reclamation/.build/I3_rcu_student.exe
$LASTEXITCODE
```

空 starter 的编译应成功、运行退出码应为 1；补全后全部检查通过才返回 0。核对答案时独立编译本题 solution.cpp，另用 `I3_rcu_reference` 作为输出名；它不包含 student.hpp，学生修改不会改变答案。

专题验证脚本已区分三种模式：`verify.ps1`（默认 Reference）只运行四题答案、runtime 和生命期 reference，不编译未完成 main；`verify.ps1 -Mode Starter` 检查四个空作业均在启动线程前按预期返回 1；`verify.ps1 -Mode Student` 检查四个已补全作业实际通过并返回 0。各模式可加 `-Asan`，脚本成功表示符合该模式的预期，不能把 Starter 的脚本成功理解为作业完成。直接编译与本次复审证据见 [验证记录](../../topics/reclamation/06-validation.md)。

检查使用 cs::check，Release 下仍执行。MSVC 的学生入口仅抑制 C4702：故意抛异常的 TODO 在 /O2 下会让编译器识别出成功分支不可达；其他 /W4 /WX 检查保留。这个诊断设置不决定通过结果。

## 共用设施与责任

默认 C++23，源指针全部使用 SC。回收库、读区协议、门闩异常传播及输入生成已提供，不要求重写公共算法。用户操作仍须遵守正文的同域、不可变载荷、一次退休和保护边界约束；完整操作不承诺 lock-free。空 TODO 的预检是安全入口约束，不是对任意错误学生代码的内存安全保证。错误实现应另用 ASan 和外部超时诊断。

依赖：C++23 标准库、线程支持及 exercises/include；I3/R1/R2 的检查器复用 topics/reclamation/experiment_support.hpp。没有新链接库。main 与 solution 必须分别编译为独立程序，不能链接两个 main。
