# EBR：独立学生 pin、退休与退出

完整推导、时序与来源见 [专题正文](../../topics/reclamation/03-epoch-reclamation.md)。

## 本题任务与验收

| Part | 只在 student.hpp 补全 | 检查器实际检查 |
|---|---|---|
| A | pin(domain)：返回现成 epoch_guard | 最外层 pin 生效；内层退出不能取消外层保护 |
| B1 | retire(unique_ptr<value>, domain) | 已摘除对象交给域；不会在长读者存活时直接析构 |
| B2 | collect(domain) | 长读者未退出时返回 0；退出后实际回收 64 份 |
| C | shutdown(source, domain) | join 后清源、退休最终版本、等待回调排空 |

pin 返回 prvalue，由 C++ 的复制消除直接构造调用方 guard；不需要让公共 epoch_guard 变成可移动，也不需要手写登记表。source 的输入替换、唯一所有权转交、读者门闩和异常收尾由检查器提供；学生补的是调用已有协议的操作。

预检在一个真实嵌套读区中退休对象，检查保护仍在，再离开外层验证回收和 shutdown。并发阶段 worker 先调用学生 pin、读取 42 并等待；主线程固定退休 64 份，调用学生 collect 必须没有删除。开放门闩并 get 收束读者后，学生 collect 回收 64，shutdown 完成第 65 次删除。逻辑 int 载荷预算 64*sizeof(int)，不含观测字段、分配器和回收元数据。

自测答案：代号增长不等于旧读者已退出；只结束内层不够；线程退出以后仍需完成回调。见正文第 1—7 节。不要编辑 reference.hpp 的分代实验或公共 epoch.hpp；输入预算和读者启动/收尾框架无需学生重写。

## 编辑边界与运行方式

这是独立的学生补全题。只编辑本题 `student.hpp` 的 TODO；`main.cpp → checks.hpp → student.hpp` 是作业路径，`solution.cpp → reference.hpp` 是只读答案路径。检查器不会调用 reference::run，也不让学生改 reference.hpp、检查器、公共回收头或完成标志。没有“完成=true”开关。

每个 TODO 默认抛出带位置的 logic_error。main 先执行单线程 preflight，实际检查每个学生操作的效果；有任一步未填或预检失败，在线程启动前输出 student FAIL 并返回 **1**。不是 77。只有预检通过才出现 `preflight PASS; starting student workers`，并启动并发检查。删除异常、返回常量或把操作改为空函数，不能替代对应的状态变化与回调结果。

在仓库根目录的 x64 MSVC Developer PowerShell 编译本题 main（把下方题目 ID 代入路径）：

```powershell
New-Item -ItemType Directory -Force Concurrency_Study/topics/reclamation/.build | Out-Null
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG /IConcurrency_Study/exercises/include /FoConcurrency_Study/topics/reclamation/.build/R1_epoch_reclamation_student.obj /FeConcurrency_Study/topics/reclamation/.build/R1_epoch_reclamation_student.exe Concurrency_Study/exercises/R1_epoch_reclamation/main.cpp
./Concurrency_Study/topics/reclamation/.build/R1_epoch_reclamation_student.exe
$LASTEXITCODE
```

空 starter 的编译应成功、运行退出码应为 1；补全后全部检查通过才返回 0。核对答案时独立编译本题 solution.cpp，另用 `R1_epoch_reclamation_reference` 作为输出名；它不包含 student.hpp，学生修改不会改变答案。

专题验证脚本已区分三种模式：`verify.ps1`（默认 Reference）只运行四题答案、runtime 和生命期 reference，不编译未完成 main；`verify.ps1 -Mode Starter` 检查四个空作业均在启动线程前按预期返回 1；`verify.ps1 -Mode Student` 检查四个已补全作业实际通过并返回 0。各模式可加 `-Asan`，脚本成功表示符合该模式的预期，不能把 Starter 的脚本成功理解为作业完成。直接编译与本次复审证据见 [验证记录](../../topics/reclamation/06-validation.md)。

检查使用 cs::check，Release 下仍执行。MSVC 的学生入口仅抑制 C4702：故意抛异常的 TODO 在 /O2 下会让编译器识别出成功分支不可达；其他 /W4 /WX 检查保留。这个诊断设置不决定通过结果。

## 共用设施与责任

默认 C++23，源指针全部使用 SC。回收库、读区协议、门闩异常传播及输入生成已提供，不要求重写公共算法。用户操作仍须遵守正文的同域、不可变载荷、一次退休和保护边界约束；完整操作不承诺 lock-free。空 TODO 的预检是安全入口约束，不是对任意错误学生代码的内存安全保证。错误实现应另用 ASan 和外部超时诊断。

依赖：C++23 标准库、线程支持及 exercises/include；I3/R1/R2 的检查器复用 topics/reclamation/experiment_support.hpp。没有新链接库。main 与 solution 必须分别编译为独立程序，不能链接两个 main。
