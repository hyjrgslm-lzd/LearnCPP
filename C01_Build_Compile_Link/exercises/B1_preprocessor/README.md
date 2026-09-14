# B1_preprocessor

本练习验证预处理是每个翻译单元各自发生的文本转换。`macro_a.cpp` 和 `macro_b.cpp` 包含同一个 `generated_value.hpp`，但在包含前定义不同 `B1_LOCAL_OFFSET`。头里的演示函数使用 `static inline`，保证每个翻译单元得到内部 linkage 定义；如果写成外部 `inline`，两个不同函数体会违反 ODR。

学生只修改 `src/student/student_value.cpp`。普通默认配置不注册学生测试；`vs-study` 和 `student` 预设会注册，未完成的 starter 应失败。

## VS 日常入口

先按[总构建指南](../BUILD_GUIDE.md)生成并构建 `vs-study`，打开整章解决方案并选择 `Debug | x64`。本节命令从 `C01_Build_Compile_Link/exercises` 目录执行。

将 `B1_preprocessor_student` 设为启动项目，在 `Student` 中编辑 [student_value.cpp](src/student/student_value.cpp)；`student_value.hpp`、`Checks/student_check.cpp` 和 `Common/check.hpp` 都已列入同一项目。参考项目位于本题的 `Reference`，其中两个宏示例仍分别编译为独立翻译单元。

除明确标注的独立工具步骤外，下文命令也从 `exercises` 目录执行，使用已构建的 `vs-study`。

## Part 1：两个翻译单元的宏状态

`macro_a.cpp`：

```cpp
#define B1_LOCAL_OFFSET 11
#include "generated_value.hpp"
```

`macro_b.cpp`：

```cpp
#define B1_LOCAL_OFFSET 22
#include "generated_value.hpp"
```

Reference 检查：

- `macro_value_from_a() == 118`
- `macro_value_from_b() == 129`
- 两个返回值不同

**解析：** 宏状态不是全项目共享变量。每个 `.cpp` 独立预处理，包含同一头文件也会根据当前翻译单元已有宏得到不同文本。这里额外加了 `pragma_once_example_value() == 7`，让最终值来自“宏展开结果 + 普通 inline helper”，检查不是只搜字符串。

## Part 2：保存预处理输出

运行：

```powershell
ctest --preset vs-study -R '^B1_preprocessor_preprocess$'
```

测试会调用当前 C++ 编译器预处理两个源文件，并保存：

- `build/vs-study/B1_preprocessor/evidence/preprocess_macro_a.txt`
- `build/vs-study/B1_preprocessor/evidence/preprocess_macro_b.txt`

期望文本分别包含 `return ((100) + 11)` 和 `return ((100) + 22)`。

**解析：** 预处理输出说明编译器真正看到的文本。它能证明宏展开与条件分支选择，但不能证明链接是否成功，也不能证明运行行为。运行行为由 Reference executable 检查。

## Part 3：include guard 与 `#pragma once`

`generated_value.hpp` 使用标准 include guard。`pragma_once_example.hpp` 使用 `#pragma once`。

**解析：** include guard 是标准写法，限制同一次预处理里同一头文件内容重复进入。`#pragma once` 是常见编译器扩展，省掉宏命名冲突，但依赖编译器识别文件身份。两者都只作用于单个翻译单元；它们不会告诉另一个 `.cpp` “这个头已经包含过”。跨 TU 的重复定义问题属于 ODR/链接章节。

## Part 4：Student

`vs-study` 已注册学生检查；修改实现后重新构建并运行：

```powershell
cmake --build --preset vs-study --target B1_preprocessor_student
ctest --preset vs-study -R '^B1_preprocessor_student$'
```

starter 应失败。完成条件：`b1_student::configured_value() == 123`。

**解析：** 本题学生实现只需要安全地给出目标值，不要求写宏。宏是观察对象，不是鼓励用宏替代函数和常量。若确实使用宏，也应把它限制在 `.cpp` 内，避免污染公共头和消费者翻译单元。
