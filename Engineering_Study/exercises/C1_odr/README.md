# 练习 C1-ODR：头文件定义、重复符号与两种修复

先读 [02：ODR、符号与头文件里的定义](../../chapters/02-odr-and-symbols.md)。本题只研究一个小函数：

```cpp
int lesson_value();
```

最终 `lesson_value()` 和两个调用路径都必须返回 `42`。检查器和调用者不属于学生编辑区；学生只改 `src/student/lesson.cpp`。这样可以防止把 `main` 或 caller 改成直接成功，而没有修复真实实现。

## Part 1：观察单消费者头文件定义

构建后运行：

```powershell
ctest --test-dir build --tests-regex C1_odr_observation_single --output-on-failure
```

这个程序只有一个 `.cpp` 包含头文件函数定义，应该输出：

```text
single consumer header definition: 42
```

**解析：** 只有一个翻译单元时，程序里只有一份普通外部函数定义。这个阶段合法，不代表头文件函数体能安全被多个 `.cpp` 包含。

## Part 2：复现第二个翻译单元导致的重复符号

负例不进入默认 build。用 CTest 触发嵌套构建：

```powershell
ctest --test-dir build --tests-regex C1_odr_negative --output-on-failure
```

两个负例都必须“构建失败才算通过测试”：

- `negative/duplicate_header_definition`：两个 `.cpp` 包含同一个普通函数定义头文件。
- `negative/include_guard_same_failure`：同样有 include guard，但仍然失败。

**解析：** include guard 只限制同一个 `.cpp` 的预处理结果。两个 `.cpp` 会分别得到函数体，并分别生成同名外部定义。测试先构建两个 object target，确认负例已经通过编译；再单独构建 link target，确认失败来自链接阶段的 `lesson_value` 重复定义。MSVC 常见诊断包含 `LNK2005`/`LNK1169`，GCC/Clang 常见诊断包含 `multiple definition`。编译期 `#error`、configure 失败、timeout 或缺失符号不能算作本题负例通过。

## Part 3：查看预处理与符号证据

运行：

```powershell
ctest --test-dir build --tests-regex C1_odr_observation_ --output-on-failure
```

`C1_odr_observation_preprocess` 会调用当前 C++ 编译器预处理 `observer_a.cpp`，验证输出里确实有 `int lesson_value()` 的函数体。`C1_odr_observation_symbols` 会构建两个 object 文件，并用 Windows `dumpbin /symbols` 或 ELF `nm -C` 验证两个 object 都含有 `lesson_value` 定义。

**解析：** 预处理证据说明头文件文本进入翻译单元；符号证据说明每个 object 都产生了可链接定义；链接负例说明这些定义不能共同组成一个普通程序。三种证据不要混用。

## Part 4：修复 Student

打开 `src/student/lesson.cpp`。初始状态会构建成功，但 `C1_odr_student` 运行返回非零，因为 `lesson_value()` 还没有返回 `42`。请选择一种修复方式：

### 方案 A：声明 + `.cpp` 单一定义

`lesson.hpp` 只保留声明：

```cpp
int lesson_value();
```

`lesson.cpp` 放唯一函数体：

```cpp
int lesson_value()
{
    return 42;
}
```

### 方案 B：inline 头文件定义

`lesson.hpp` 放 `inline` 定义：

```cpp
inline int lesson_value()
{
    return 42;
}
```

当前 Starter 的 CMake 默认采用方案 A 的结构；本题实际学生入口只要求修改 `lesson.cpp`。方案 B 用于理解另一条正确路径，对应完整 Reference 在 `reference/inline_definition`。

完成后运行：

```powershell
ctest --test-dir build --tests-regex C1_odr_student --output-on-failure
```

**解析：** 检查器会直接调用 `lesson_value()`，再分别调用 `call_lesson_from_a()` 和 `call_lesson_from_b()`。直接实现和两条路径都必须返回 `42`，总和必须是 `84`。只让 caller 直接 `return 42`、只改输出文字、只在检查器里硬编码，都不能通过真实检查。

## Reference

Reference 提供两种独立正确修复：

- `reference/split_definition`：声明放头文件，函数体放一个 `.cpp`，目标名 `C1_odr_reference`。
- `reference/inline_definition`：头文件使用 `inline` 定义，目标名 `C1_odr_reference_inline`。

运行：

```powershell
ctest --test-dir build --tests-regex C1_odr_reference --output-on-failure
```

两者都应通过。

## 独立构建

```powershell
cmake -S Engineering_Study/exercises/C1_odr -B Engineering_Study/exercises/build/sample-odr -G Ninja
cmake --build Engineering_Study/exercises/build/sample-odr
ctest --test-dir Engineering_Study/exercises/build/sample-odr --output-on-failure
```

当前 Starter 的完整 `ctest` 会因为 `C1_odr_student` 失败而返回非零，这是预期的学生未完成状态。作者验收应分别确认：

- reference 通过。
- observation 通过。
- negative 嵌套构建按预期失败并被测试捕获。
- student starter 返回非零。

GCC/ELF 路径保留了 `nm -C` 观察逻辑，但本样章主验收环境是 Windows/MSVC。

