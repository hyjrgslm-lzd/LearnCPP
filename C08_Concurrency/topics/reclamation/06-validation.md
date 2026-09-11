# 回收专题：验证方法

本页保留回收专题的复查方法。原始命令输出、临时完成副本、文件指纹、作者复测和审查记录属于过程产物，不作为课程源码提交。

## 学生入口与只读答案

I2/I3/R1/R2 的学生入口只编辑 `student.hpp`。`main.cpp` 包含本题 `checks.hpp`，实际调用学生操作；`solution.cpp` 与 `reference.hpp` 是只读答案路径，不应和学生程序链接进同一个可执行文件。

专题脚本区分三种模式：

```powershell
./C08_Concurrency/topics/reclamation/verify.ps1
./C08_Concurrency/topics/reclamation/verify.ps1 -Mode Starter
./C08_Concurrency/topics/reclamation/verify.ps1 -Mode Student
./C08_Concurrency/topics/reclamation/verify.ps1 -Asan
```

- 默认模式只运行 Reference、runtime 和生命周期参考程序。
- Starter 模式要求空作业返回 1，输出 TODO 诊断，并且不启动 worker。
- Student 模式要求补全后的学生操作真实通过检查并返回 0。
- `-Asan` 只说明本次地址访问路径无报告，不替代数据竞争、进展或弱内存验证。

## 单题直接编译

从仓库根目录的 x64 MSVC Developer PowerShell 编译一题：

```powershell
New-Item -ItemType Directory -Force C08_Concurrency/topics/reclamation/.build | Out-Null
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG /IC08_Concurrency/exercises/include /FoC08_Concurrency/topics/reclamation/.build/I2_student.obj /FeC08_Concurrency/topics/reclamation/.build/I2_student.exe C08_Concurrency/exercises/I2_hazard_pointer/main.cpp
./C08_Concurrency/topics/reclamation/.build/I2_student.exe
cl /nologo /std:c++23preview /EHsc /W4 /WX /utf-8 /O2 /DNDEBUG /IC08_Concurrency/exercises/include /FoC08_Concurrency/topics/reclamation/.build/I2_reference.obj /FeC08_Concurrency/topics/reclamation/.build/I2_reference.exe C08_Concurrency/exercises/I2_hazard_pointer/solution.cpp
./C08_Concurrency/topics/reclamation/.build/I2_reference.exe
```

ASan 增加 `/fsanitize=address /Z7`，运行进程需能从 `PATH` 找到对应工具链的 ASan DLL。GCC/Clang 可用等价的 `-std=c++23 -O2 -DNDEBUG -pthread -I...` 形式；未实际运行的平台不能写成通过。

## 检查范围

| 场景 | 必须观察到的结果 |
|---|---|
| 生命周期模型 | 安全模型达到反例状态，不真正解引用悬垂指针 |
| hazard pointer | 保护期间不删除；reset、move、槽耗尽和 cleanup 都有明确效果 |
| epoch/RCU/QSBR | 长读者阻挡旧记录，offline/quiescent/synchronize/barrier 的边界分开 |
| 学生栈/配置练习 | ID 恰好一次、析构数匹配、pending 归零 |
| 故障路径 | 注册、分配、报告、回调和启动失败时释放已接受 worker 与已发布对象 |

门闩只制造确定阶段。动态检查不能穷尽全部调度、真实内存耗尽、线程创建器实际故障、TSan/弱内存硬件或模型检查器结论。性能、公平性、最大延迟和吞吐排名不在本页验证范围。

## 交付边界

源码和课程文档保留；`.build`、临时完成副本、stdout/stderr、JSON、SHA 清单和审查记录留在本机验证目录。学生 README 应说明只改 `student.hpp`，不能要求学生重写共享 `reference.hpp`。
