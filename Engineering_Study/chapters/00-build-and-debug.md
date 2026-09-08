# 00. 从一个源文件到可调试程序

C++ 工程的第一条链路不是“写代码然后点运行”。编译器只能处理翻译单元，链接器只看 object 和库里的符号，调试器只能解释当前可执行文件里的调试信息。把这几层分清，后面的 CMake、库、ODR 和 ABI 问题才有可定位的边界。

本章从一个 `.cpp` 文件开始，逐步走到配置构建、运行和调试。目标不是记住某个 IDE 按钮，而是能说清：当前命令做了哪一步、产物在哪里、失败属于编译还是链接还是运行、Debug 和 Release 改变了什么。

## 单文件编译：先得到一个可执行文件

考虑最小程序：

```cpp
#include <iostream>

int make_answer(int base)
{
    int adjusted = base + 2;
    return adjusted * 2;
}

int main()
{
    int seed = 19;
    std::cout << make_answer(seed) << '\n';
}
```

直接用编译器可以一步完成编译和链接：

```powershell
g++ -std=c++23 -g -O0 main.cpp -o a1.exe
.\a1.exe
```

这条命令里有两个动作。`g++` 先把 `main.cpp` 编译成临时 object，再调用链接器把 object 和 C++ 运行库链接为 `a1.exe`。`-std=c++23` 选择语言模式；`-g` 写入调试信息；`-O0` 关闭优化，方便调试器把机器指令对应回源码行和局部变量。

如果源文件里少了分号，失败发生在编译阶段；如果声明了函数但没有定义，编译可以过，链接会失败。阶段不同，修复方向也不同。编译器不负责在另一个 `.cpp` 里帮你找函数体；链接器也不理解模板约束、宏展开或源代码缩进。

## 从命令到 CMake：配置不是构建

真实工程不会手写每个编译命令。CMake 的职责是生成构建系统，而不是直接替代编译器。

```powershell
cmake -S A1_build_debug -B build/a1 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/a1
ctest --test-dir build/a1 --output-on-failure
```

第一行是配置：CMake 读取 `CMakeLists.txt`，选择生成器，探测编译器，写出 Ninja 或 Visual Studio 工程。第二行才是构建：它调用生成器，再由生成器调用编译器和链接器。第三行运行测试。配置成功只说明“可以生成构建规则”；构建成功只说明“目标产物被生成”；测试通过才说明“被检查的程序行为符合检查”。

Ninja 是单配置生成器，`CMAKE_BUILD_TYPE=Debug` 在配置阶段决定默认配置。Visual Studio 是多配置生成器，配置时生成一个工程，构建时用 `--config Debug` 或 `--config Release` 选择产物。把这两个模型混用，会出现“以为在 Debug，实际跑的是 Release”的错觉。

## 执行：程序状态来自代码路径

调试前先让程序自己可检查。`A1_build_debug_reference` 不只返回 0，它会检查两次调用结果、局部状态传播和调用路径。这样的检查比“能运行”更强，因为它消费了实际函数返回值：如果把核心函数改成常量、跳过调用或初始化错变量，检查会失败。

变量初始化和作用域是调试的第一组事实。局部变量进入作用域后才存在；初始化语句执行前，调试器显示的值不能当成程序语义。出了作用域，变量名不再可用；编译器在优化模式下还可能把变量放进寄存器、常量折叠或完全消除。

所以调试时要先停在有意义的位置。断在 `main` 的第一行，只能证明程序入口被命中；执行过 `int seed = 19;` 后再看 `seed`，才是在观察已初始化对象。

## 调试调用链：断点、单步、栈和局部变量

GDB 的最小闭环如下：

```gdb
break main
run
next
step
backtrace
info locals
```

`break main` 让程序停在入口；`run` 启动程序；`next` 执行当前行但不进入被调用函数；`step` 进入函数调用；`backtrace` 显示当前调用栈；`info locals` 显示当前栈帧可见的局部变量。

如果 `main -> compute_answer -> add_offset` 是真实调用链，`backtrace` 应能看到这些函数。若检查只看最终输出 42，学生可以把 `main` 改成直接打印 42；若检查还消费调用路径和中间值，这种绕过就会暴露。

本课程在 Windows 上用 MinGW g++13.2 + GDB14.2 观察 `-g -O0` 产物：可以断在 `main`，单步进入函数，查看调用栈和局部变量。这个事实只证明该工具链路径可用，不证明 GDB 能完整解释 MSVC PDB，也不证明 Release 优化后每个局部变量都可见。

## Debug 和 Release 的区别

Debug 通常保留更多调试信息，少做或不做优化，变量和调用结构更接近源码。Release 通常打开优化，删除死代码，内联小函数，合并常量，重排指令。Release 不是“没有检查”；本课程的 `check()` 在 Release 下仍有效，避免把 `assert` 被 `NDEBUG` 关闭后的沉默当成正确。

优化不会改变有定义行为的可观察结果，但会改变你在调试器里看到的路径。一个变量可能显示为 optimized out；一个函数可能没有单独栈帧；一行源码可能对应多段指令。遇到这类现象，先用 Debug 复现实验，再用 Release 证明最终行为，而不是用 Release 调试器视图反推源码没有执行。

## A1 练习如何验证

A1 的 Reference 检查固定调用链和结果。Observation 检查构建配置传入的宏和当前编译模式。Student 目标默认不注册测试；显式打开学生测试时，starter 会失败，要求学生修改自己的实现文件，而不是改检查器或 Reference。

验收证据包括：单文件 `g++ -g -O0` 构建运行、GDB 断点/单步/栈/局部变量输出、CMake Debug/Release 构建与 CTest。MSVC 与 MinGW 的结论分开记录：MSVC 证明课程 CMake 目标可构建运行；GDB 证明 MinGW 调试链可观察源码级状态。
