# B1_preprocessor

本练习验证预处理是每个翻译单元各自发生的文本转换。`macro_a.cpp` 和 `macro_b.cpp` 包含同一个 `generated_value.hpp`，但在包含前定义不同 `B1_LOCAL_OFFSET`。头里的演示函数使用 `static inline`，保证每个翻译单元得到内部 linkage 定义；如果写成外部 `inline`，两个不同函数体会违反 ODR。

学生只修改 `src/student/student_value.cpp`。starter 默认不注册测试；打开 student 测试后会失败。

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
ctest --test-dir build/b1 --tests-regex B1_preprocessor_preprocess --output-on-failure
```

测试会调用当前 C++ 编译器预处理两个源文件，并保存：

- `build/b1/evidence/preprocess_macro_a.txt`
- `build/b1/evidence/preprocess_macro_b.txt`

期望文本分别包含 `return ((100) + 11)` 和 `return ((100) + 22)`。

**解析：** 预处理输出说明编译器真正看到的文本。它能证明宏展开与条件分支选择，但不能证明链接是否成功，也不能证明运行行为。运行行为由 Reference executable 检查。

## Part 3：include guard 与 `#pragma once`

`generated_value.hpp` 使用标准 include guard。`pragma_once_example.hpp` 使用 `#pragma once`。

**解析：** include guard 是标准写法，限制同一次预处理里同一头文件内容重复进入。`#pragma once` 是常见编译器扩展，省掉宏命名冲突，但依赖编译器识别文件身份。两者都只作用于单个翻译单元；它们不会告诉另一个 `.cpp` “这个头已经包含过”。跨 TU 的重复定义问题属于 ODR/链接章节。

## Part 4：Student

打开 student 测试：

```powershell
cmake -S B1_preprocessor -B build/b1-student -G Ninja -DENGINEERING_STUDY_TEST_STUDENTS=ON
cmake --build build/b1-student
ctest --test-dir build/b1-student -L student --output-on-failure
```

starter 应失败。完成条件：`b1_student::configured_value() == 123`。

**解析：** 本题学生实现只需要安全地给出目标值，不要求写宏。宏是观察对象，不是鼓励用宏替代函数和常量。若确实使用宏，也应把它限制在 `.cpp` 内，避免污染公共头和消费者翻译单元。
