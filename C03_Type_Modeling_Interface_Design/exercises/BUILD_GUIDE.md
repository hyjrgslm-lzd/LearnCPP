# 构建、练习与证据

## 本机入口

需要完整 LearnCPP checkout、CMake 4.2+、Visual Studio 2026 C++ 工具链和 Python 3.10+。课程CMake脚本的最低版本为3.28，但Windows预设使用4.2才加入的 `Visual Studio 18 2026` / x64生成器；两项要求不同。课程核心C++23，无下载步骤。在本目录运行：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core
ctest --preset verify-core
cmake --preset verify-debug
cmake --build --preset verify-debug
ctest --preset verify-debug
```

配置成功只说明生成成功；build和ctest必须分别执行。其他平台可显式选择自己的生成器，例如 `cmake -S . -B build/unix -G Ninja -DCMAKE_BUILD_TYPE=Release`，但本课程的本机报告不代表这些平台已测试。

## Student、答案与观察

实现题的 `src/student` 是学生编辑位置，`src/reference` 是独立答案；`checks` 调用当前选择的实现。`validation/good` 和 `validation/bad` 检验检查器本身，不能修改学生文件来构造通过。观察型程序原本可运行，运行成功只代表执行过其检查，不代表预测、解释和扩展任务完成。

```powershell
cmake --preset student
cmake --build --preset student
ctest --preset student
```

初始Student未实现，预期以明确检查失败退出；这是正常学习起点，不应通过改完成标记、跳过或链接Reference使它变绿。完成自己的代码后再运行相同检查。单题以L06为例：

```powershell
cmake -S L06_transactions -B build/lesson-L06 -G "Visual Studio 18 2026" -A x64
cmake --build build/lesson-L06 --config Release
ctest --test-dir build/lesson-L06 -C Release --output-on-failure
```

默认Student目标存在但不进入默认build；可显式 `--target L06_transactions_student`。整课 `c03_students` 一次构建所有Student目标。Student预设关闭全部Reference与validation答案目标，审计另检查实际预处理include和链接接线，不声称能发现任意抄写算法。

## 安全诊断与前沿

```powershell
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
cmake --preset frontier
cmake --build --preset frontier
ctest --preset frontier
```

Windows ASan使用MSVC RelWithDebInfo，构建时从当前cl目录复制匹配的ASan DLL到目标目录，不能借用另一版同名DLL。默认运行reference/observation/capability/allocation安全标签；ASan无报告不构成无UB证明。P1普通Debug保留默认迭代器诊断，全局分配注入仅在单独allocation目标运行，原因与边界见[P1说明](P1_document/README.md)。危险反例默认关闭，不将空调用强前置条件或实际悬垂作为普通成功用例。

新标准的真实示例与逐项能力边界见 [F01](F01_frontier/README.md)。启用选项、宏存在、实例化成功、链接成功与运行结果是不同证据。能力不可用可明确SKIP，有能力后的真实错误不能被吞为SKIP。

## 可复现日志

使用现有 [record_process.py](../../C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py) 记录有界子进程。其底层 [process_runner.py](../../C01_Build_Compile_Link/exercises/tools/process_runner.py) 只管理自己启动的进程；现有文件拒绝被覆盖，重跑使用新证据名。

```powershell
python ../../C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py --output ../../build/local-records/c03-release.json --timeout 180 -- ctest --preset verify-core
```

测试一般外部超时30秒，编译/构建驱动外部超时180秒或记录的有界上限。负例必须匹配预期阶段、退出码和诊断，超时、缺DLL、启动/清理失败都是真失败。校验函数复用 [check.hpp](../../C01_Build_Compile_Link/exercises/include/check.hpp)，Release下仍执行。

Student接线复用 [audit_student.py](../../C02_Objects_Lifetime_Ownership/exercises/tools/audit_student.py)：配置前请求CMake codemodel-v2，记录所有Student目标的 `--clean-first` `/showIncludes` 构建，再按配置审计。历史运行记录不能代替当前版本重跑。
