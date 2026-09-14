# C18 构建与运行

以下命令以课程目录 `C18_Interoperability_Plugins_Tooling` 为当前目录。核心要求 CMake>=3.24、C11、C++23及可运行的Python检查驱动。Windows采用 Visual Studio 18 2026生成器时需要支持该生成器的CMake，示例环境为CMake4.2系列。独立free-threaded单元另要求CMake>=3.30。

## 核心路径：离线配置、真实构建、再运行

```powershell
cmake -S exercises -B build/msvc -G "Visual Studio 18 2026" -A x64
cmake --build build/msvc --config Release
python tools/run_check.py --timeout 300 -- ctest --test-dir build/msvc -C Release --output-on-failure
cmake --build build/msvc --config Debug
python tools/run_check.py --timeout 300 -- ctest --test-dir build/msvc -C Debug --output-on-failure
```

`Configuring done` 只证明工程生成；必须看到实际目标编译/链接，再运行检查。不要强行设置 compiler works、绕过缺库，或用缓存中的旧二进制冒充本次验证。更换生成器/工具链时使用新构建目录。MSVC对请求的C++23可能生成 `/std:c++latest`，以实际编译日志为准，课程不由此宣称使用了未来全部特性。

Linux/WSL 核心命令：

```sh
cmake -S exercises -B build/linux-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/linux-release
python3 tools/run_check.py --timeout 300 -- ctest --test-dir build/linux-release --output-on-failure
cmake -S exercises -B build/linux-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/linux-debug
python3 tools/run_check.py --timeout 300 -- ctest --test-dir build/linux-debug --output-on-failure
```

WSL可直接在 `/mnt/f/CPPTrain/LearnCPP/C18_Interoperability_Plugins_Tooling` 读取源码；构建目录也可以选本机允许的独立Linux位置。结果只代表相应平台。需要Sanitizer时先验证编译/链接及最小运行能力；能力具备后的主体错误必须修复，不能改成SKIP。

ASan/UBSan使用独立核心构建，只检查原生Reference/good、P1原生路径、转换正确性及输出保护。Python宿主加载的扩展、解释器/AST观察和故意bad控制由普通矩阵独立验证，不把它们混成已完成的Sanitizer验证：

```sh
cmake -S exercises -B build/linux-sanitizer -G Ninja -DCMAKE_BUILD_TYPE=Debug -DC18_ENABLE_SANITIZERS=ON
cmake --build build/linux-sanitizer
ctest --test-dir build/linux-sanitizer --output-on-failure -E '(bad_rejected|^c18_l05_|^c18_f01_|^c18_l15_|^c18_runner_classifier$)'
```

## Student 与单题构建

```powershell
cmake -S exercises -B build/student -G "Visual Studio 18 2026" -A x64 -DC18_BUILD_REFERENCE=OFF -DC18_TEST_STUDENTS=ON
cmake --build build/student --config Release
python tools/run_check.py --timeout 300 -- ctest --test-dir build/student -C Release --output-on-failure

cmake -S exercises/L04_plugin_shutdown -B build/l04 -G "Visual Studio 18 2026" -A x64
cmake --build build/l04 --config Release
ctest --test-dir build/l04 -C Release --output-on-failure
```

初始实现型Student应失败；这是未完成作业的真实状态，不是成功或SKIP。默认配置编译Student但不把它混入Reference成功集合。`C18_BUILD_REFERENCE=OFF`关闭答案与good/bad目标，保留学生所需的公共头、夹具及观察入口；Student不得依赖答案。

good是独立正确控制；bad只有正常exit1且命中指定checker诊断，才算被成功拒绝。程序缺失、崩溃、超时及无法确认清理均算真实失败。所有Release检查保持有效，不依赖会被关闭的assert。

## Python Full、Limited 与 pybind11

Full C API使用CPython3.10.11开发头/库，验证Release；不假定机器同时安装了Python Debug运行库。为避免误选Debug，使用独立配置：

```powershell
$c18Python = py -3.10 -c "import sys; print(sys.executable)"
cmake -S exercises -B build/python-full -G "Visual Studio 18 2026" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release -DC18_ENABLE_PYTHON=ON -DPython3_EXECUTABLE="$c18Python"
cmake --build build/python-full --config Release
python tools/run_check.py --timeout 300 -- ctest --test-dir build/python-full -C Release --output-on-failure
```

命令从已有Python launcher取得路径，不写死个人目录。所选解释器、头、库及位宽必须对应；其他机器按其已有安装替换。L05 ctypes无需Python开发库，但需要真实课程插件。P1的Python后端是原生C API嵌入路径，不要求pybind11。

Limited API独立见[L09](L09_limited_api/README.md)：用3.8.10头及 `Py_LIMITED_API=0x03080000` 构建一次，Windows链接 `python3.lib`，同一pyd分别交3.8.10和3.10.11加载。不能在每次运行前重新编译成不同二进制后声称跨版本成功。

pybind11固定3.0.4，入口[L10](L10_pybind11/README.md)。只有已有匹配依赖时才开启 `C18_ENABLE_PYBIND11`；源码导读和代码不等于已验证该库。free-threaded原生主体使用独立[F01](F01_runtime_frontier/README.md)构建，不与常规Full/Limited测试混用。

## Lua 与 Clang 可选开发依赖

Lua入口[L11](L11_lua_stack/README.md)固定5.4.9：

```sh
cmake -S exercises -B build/lua -DC18_ENABLE_LUA=ON -DCMAKE_PREFIX_PATH=/absolute/path/to/existing/lua-5.4.9
cmake --build build/lua --config Release
ctest --test-dir build/lua -C Release --output-on-failure
```

Lua头和库必须来自匹配构建，数字类型、调用约定及C/C++编译模式有各自约束。本课按C编译Lua的longjmp模型组织保护边界；不通过临时改成另一Lua版本或更换异常模型取得通过。

LibTooling入口[P2](P2_abi_tool/README.md)固定LLVM/Clang18.1.3：

```sh
cmake -S exercises -B build/tooling -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DC18_ENABLE_CLANG_TOOLING=ON -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm -DClang_DIR=/usr/lib/llvm-18/lib/cmake/clang
cmake --build build/tooling
ctest --test-dir build/tooling --output-on-failure
```

只有clang可执行文件或 `libclang-cpp.so` 不证明开发头和导入target齐全。显式ON缺组件必须失败，不自动下载。L15的真实AST观察独立于LibTooling主体；其通过不能替代检查、重构和生成工具的运行证据。Visual Studio生成器不提供本路线需要的编译数据库，Clang示例使用Ninja产生它。

## 环境清单、实验与证据

```sh
python tools/probe_environment.py
python tools/check_navigation.py
python tools/run_costs.py --exe build/b01/Release/c18_boundary_cost.exe --variants per_record batch copy_batch --output build/costs/run-001.json
```

环境清单只是发现文件，不是编译、链接或运行能力证明。B01执行方法见其README，先构建并通过正确性检查；计时前后源码与二进制保持一致，保留每个独立进程样本、干扰和负面结果。

正常构建不安装依赖、不改全局PATH或机器配置。测试运行器复用C01进程工具，只终止自己启动的进程；超时与清理失败保持FAIL。日志、测量、生成副本、质量报告和审查记录写 `build/`。课程源文件、固定测试夹具及可复用脚本与这些运行产物分别管理。
