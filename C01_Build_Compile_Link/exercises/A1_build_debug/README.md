# A1_build_debug

本练习把“源码 → 配置 → 构建 → 运行 → 调试”拆成可验证链路。Reference 与 Observation 默认注册；Student 默认不注册，只有显式打开 `ENGINEERING_STUDY_TEST_STUDENTS=ON` 才检查未完成 starter。

学生只修改 `src/student/debug_story.cpp`。不要改检查器、Reference 或 CMake 来绕过结果。

## Part 1：单文件编译与运行

源码：`debugger/gdb_lesson.cpp`。它包含三层调用：`main()` 初始化 `seed`，调用 `compute_answer(seed)`，再进入 `add_offset(seed, offset)`。

Windows MinGW 路径可运行：

```powershell
g++ -std=c++23 -g -O0 debugger/gdb_lesson.cpp -o build/a1-gdb.exe
.\build\a1-gdb.exe
```

期望输出 `42` 且退出码为 0。

**解析：** 这条命令让编译器完成编译和链接。`-g` 写入 GDB 可读调试信息，`-O0` 保留更接近源码的变量和调用结构。这个证据只证明 MinGW/GDB 路径，不证明 MSVC PDB 可由 GDB 完整解释。

## Part 2：CMake 配置、构建与 CTest

独立配置：

```powershell
cmake -S A1_build_debug -B build/a1 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/a1 --config Release
ctest --test-dir build/a1 --output-on-failure
```

Visual Studio generator 用：

```powershell
cmake -S A1_build_debug -B build/a1-vs -G "Visual Studio 18 2026" -A x64
cmake --build build/a1-vs --config Release
ctest --test-dir build/a1-vs -C Release --output-on-failure
```

**解析：** `cmake -S/-B` 只生成构建系统；`cmake --build` 才调用编译器和链接器；`ctest` 才运行检查。Ninja 的配置通常在 configure 阶段确定，Visual Studio 是 multi-config，实际配置来自 build/test 的 `--config` / `-C`。

## Part 3：Reference 与 Observation

`A1_build_debug_reference` 检查：

- `seed` 保存在 trace 中。
- `offset` 被初始化为 2。
- `adjusted` 来自 `add_offset()`。
- `compute_answer(19)` 得到 42，`compute_answer(20)` 得到 44。

`A1_build_debug_observation` 检查当前语言模式，并在 multi-config generator 下区分 Debug/Release 的 `NDEBUG` 状态。

**解析：** Reference 不只检查最终常量 42，而是消费两次输入和中间 trace。这样能拒绝“直接 return 42”的绕过。Observation 不替代 Reference；它只说明当前构建配置的宏状态。

## Part 4：GDB 调试调用链

脚本：`debugger/gdb_commands.txt`。

```gdb
break main
break add_offset
run
next
continue
next
backtrace
info args
info locals
```

期望在 `add_offset(seed=19, offset=2)` 处看到调用栈：`add_offset -> compute_answer -> main`，并在执行初始化语句后看到 `adjusted = 21`。

**解析：** 断在变量初始化之前看值没有语义意义。先 `next` 执行初始化，再 `info locals`，才是在观察已初始化局部变量。Release 优化下函数可能内联、变量可能 optimized out；调试定位先用 `-g -O0`，最终行为仍由 Release 检查证明。

## Part 5：Student

打开 student 测试：

```powershell
cmake -S A1_build_debug -B build/a1-student -G Ninja -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build build/a1-student
ctest --test-dir build/a1-student -L student --output-on-failure
```

starter 应失败。完成条件：`compute_answer(19) == 42` 且 `compute_answer(20) == 44`。

**解析：** 第二个输入拒绝常量答案。学生要修自己的实现，不要改检查器或链接 Reference。
