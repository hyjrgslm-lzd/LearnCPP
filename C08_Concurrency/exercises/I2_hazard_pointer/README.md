# HP：独立学生保护与安全摘除

完整推导、时序与来源见 [专题正文](../../topics/reclamation/02-hazard-pointers.md)。

## 本题任务与验收

| Part | 只在 student.hpp 补全 | 检查器实际检查 |
|---|---|---|
| A | protect(hp, source)：调用现成 HP 公布并验证源 | 返回正确源；退休后保护期间不发生删除 |
| B1 | retire(unique_ptr<node>)：把摘除节点所有权转交 HP 域 | 不允许 unique_ptr 离开作用域立即 delete；清除保护后恰好一次回调 |
| B2 | pop(head, out) 的 CAS 成功路径 | 先保护再读 next；SC CAS 失败重新保护；成功复制 int 并调用学生 retire |
| C | cleanup()：执行实际 HP 清理 | 检查真实析构数和回调数，不能只返回一个伪造数字 |

输入 push、节点类型、析构计数和异常兜底已提供，不要求再写一套栈类或扫描器。Part B 的 pop 要调用本文件的 protect/retire，不能转调 reference 的 stack。pop 成功 CAS 是成功操作的线性化点，空返回对应受保护的空源观察；节点发布后 next 不变，摘除节点不重新发布。

单线程预检检查保护/退休/清理，以及两项 LIFO 和空栈。通过后运行四个 worker，共生成 4000 个 ID；所有 pop 都经过 student::pop，排序后逐个检查 ID 恰好一次，并在检查器兜底析构之前核对学生 cleanup 已完成全部析构。总 push==总 pop 不能排除丢一个、重复另一个。

自测答案：只 load 不能建立保护；直接删除已摘除节点会伤害其他仍保护它的读者；清理返回值不能替代析构证据。对应推导见正文第 1、2、5、6 节，只读可运行答案仍在 solution.cpp/reference.hpp。

## 编辑边界与运行方式

这是独立的学生补全题。只编辑本题 `student.hpp` 的 TODO；`main.cpp → checks.hpp → student.hpp` 是作业路径，`solution.cpp → reference.hpp` 是只读答案路径。检查器不会调用 reference::run，也不让学生改 reference.hpp、检查器、公共回收头或完成标志。没有“完成=true”开关。

每个 TODO 默认抛出带位置的 logic_error。main 先执行单线程 preflight，实际检查每个学生操作的效果；有任一步未填或预检失败，在线程启动前输出 student FAIL 并返回 **1**。不是 77。只有预检通过才出现 `preflight PASS; starting student workers`，并启动并发检查。删除异常、返回常量或把操作改为空函数，不能替代对应的状态变化与回调结果。

在仓库根目录的 x64 MSVC Developer PowerShell 编译本题 main（把下方题目 ID 代入路径）：

```powershell
New-Item -ItemType Directory -Force C08_Concurrency/topics/reclamation/.build | Out-Null
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG /IC08_Concurrency/exercises/include /FoC08_Concurrency/topics/reclamation/.build/I2_hazard_pointer_student.obj /FeC08_Concurrency/topics/reclamation/.build/I2_hazard_pointer_student.exe C08_Concurrency/exercises/I2_hazard_pointer/main.cpp
./C08_Concurrency/topics/reclamation/.build/I2_hazard_pointer_student.exe
$LASTEXITCODE
```

空 starter 的编译应成功、运行退出码应为 1；补全后全部检查通过才返回 0。核对答案时独立编译本题 solution.cpp，另用 `I2_hazard_pointer_reference` 作为输出名；它不包含 student.hpp，学生修改不会改变答案。

专题验证脚本已区分三种模式：`verify.ps1`（默认 Reference）只运行四题答案、runtime 和生命期 reference，不编译未完成 main；`verify.ps1 -Mode Starter` 检查四个空作业均在启动线程前按预期返回 1；`verify.ps1 -Mode Student` 检查四个已补全作业实际通过并返回 0。各模式可加 `-Asan`，脚本成功表示符合该模式的预期，不能把 Starter 的脚本成功理解为作业完成。直接编译与本次复审证据见 [验证记录](../../topics/reclamation/06-validation.md)。

检查使用 cs::check，Release 下仍执行。MSVC 的学生入口仅抑制 C4702：故意抛异常的 TODO 在 /O2 下会让编译器识别出成功分支不可达；其他 /W4 /WX 检查保留。这个诊断设置不决定通过结果。

## 共用设施与责任

默认 C++23，源指针全部使用 SC。回收库、读区协议、门闩异常传播及输入生成已提供，不要求重写公共算法。用户操作仍须遵守正文的同域、不可变载荷、一次退休和保护边界约束；完整操作不承诺 lock-free。空 TODO 的预检是安全入口约束，不是对任意错误学生代码的内存安全保证。错误实现应另用 ASan 和外部超时诊断。

依赖：C++23 标准库、线程支持及 exercises/include；I3/R1/R2 的检查器复用 topics/reclamation/experiment_support.hpp。没有新链接库。main 与 solution 必须分别编译为独立程序，不能链接两个 main。
