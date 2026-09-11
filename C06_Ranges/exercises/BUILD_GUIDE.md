# C06构建与验证

默认保持C++26。本批主验收环境为Windows x64、Visual Studio 18 2026、MSVC19.51、STL145/202604、CMake4.2.3。其他历史预设保留，不意味着本批完成相应平台验证。CMake声明最低3.28，但使用VS2026生成器需要识别该生成器的版本，本机验证版本为4.2.3。

所有命令从仓库根开始，先进入练习目录：

```powershell
cd C06_Ranges/exercises
cmake --preset vs2026
cmake --build --preset vs2026 --parallel 2
ctest --preset core-release
cmake --build --preset vs2026-debug --parallel 2
ctest --preset core-debug
```

Reference、独立good、bad拒绝、观察程序和B01正确性快测构成普通验证。Release检查使用已有C01 `check.hpp`，不因NDEBUG消失。默认未完成Student不加入该通过数。

## 单题与Student

```powershell
cmake --build build-vs2026 --config Release --target G1_my_take_view_student
./build-vs2026/G1_my_take_view/Release/G1_my_take_view_student.exe
```

初态应输出具体`check failed: ...`并返回1；它不是安装失败，也不能改成SKIP。编辑该题`src/student/`，重编后用相同checker验证。旧题名例如`--target G1_my_take_view`保留为Student构建入口，实际程序名带`_student`。

单题也可以独立配置，不依赖先生成整课：

```powershell
cmake -S G1_my_take_view -B G1_my_take_view/build-local -G "Visual Studio 18 2026"
cmake --build G1_my_take_view/build-local --config Release --parallel 2
ctest --test-dir G1_my_take_view/build-local -C Release --output-on-failure
```

同一build目录不要同时启动两个MSBuild进程；一次build可用`--parallel`，多个独立作者应使用不同的build目录。`.slnx`、exe、PDB和CMake缓存属于本地生成物，不随课程证据发布。

## ASan安全集

```powershell
cmake --preset vs2026-asan
cmake --build --preset vs2026-asan --parallel 2
ctest --preset asan
```

ASan预设使用RelWithDebInfo，避免把普通Debug的运行时检查组合当成已经支持的ASan配置。公共helper选择与编译器匹配的运行库并复制到程序目录，缺少运行库会明确失败；不修改机器PATH或安装组件。默认只执行Reference、good、观察与正确性快测，排除negative标签；不运行冻结的真实UB旧代码。

ASan无报告只覆盖实际执行路径；它不证明借用、异常或全部输入都正确。MSVC不提供UBSan；显式请求该选项会报清楚的配置错误，不能假装启用了检测。

## 前沿能力

```powershell
cmake --preset vs2026-frontier
cmake --build --preset vs2026-frontier --parallel 2
ctest --preset frontier
```

F01默认的flat容器是普通验证；十个C++26/C++29主体单独启用。头文件、宏版本或受约束重载缺失时返回77，CTest显示SKIP；进入真实主体后编译、链接或运行失败就是FAIL。`100% tests passed`可能包含SKIP，必须另列通过和跳过数量。关闭选项不算能力探测成功，也不算十次SKIP。

## Student-only接线审计

```powershell
cmake --preset vs2026-student
cmake --build --preset vs2026-student --parallel 2
ctest --preset students
```

此配置关闭Reference与good/bad，启用Student测试；未完成初态会让CTest返回失败，这是单独记录的拒绝边界。B01依赖教师Reference，随Reference选项关闭，不能将B01未运行计为通过。include跟踪由此预设的`/showIncludes`与局部`VSLANG=1033`提供；完整审计还需CMake file-api和全Student显式重编记录，输出留在本地未跟踪目录。

## 可复现证据与性能

复用C02的`exercises/tools/record_process.py`保存每条命令、cwd、输出、退出码、进程外超时和清理状态；记录文件名必须新建并放在本地未跟踪目录。C02的`audit_student.py`复核file-api、真实includes和源接线；链接复核可以使用本地脚本或普通Markdown链接检查，均不代替教学审查。

正式性能实验见[B01题面](B01_cost/README.md)。先通过正确性快测和诊断，再在所有构建停止、源码与exe冻结时运行一次预热加五次独立进程采样。不能拿作者smoke样本或与并行编译重叠的结果作正式排名。

## 常见判断错误

- CMake配置/生成成功不等于编译出exe；构建成功不等于程序行为通过。
- Student和bad的拒绝需要指定失败原因；任意非零退出、超时、ASan崩溃不算相同证据。
- feature宏只能表示库声明；缺能力的预处理分支内主体没有被本机实例化，不能写成“所有前沿代码均验证”。
- 验证只证明被执行的检查；题目中的预测、解释、结构推导和源码理解仍需要非作者教学审查。
