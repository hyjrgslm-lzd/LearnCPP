# C2_archive

本练习验证 object、符号和静态归档成员按需抽取。

学生只修改 `src/student/archive_value.cpp`。GCC/ELF 的 `nm`/`ar` 路径保留在代码和正文中；未在当前机器运行的平台路径不能写成已运行。

## VS 日常入口

先按[总构建指南](../BUILD_GUIDE.md)生成并构建 `vs-study`，打开整章解决方案并选择 `Debug | x64`。本节命令从 `C01_Build_Compile_Link/exercises` 目录执行。

将 `C2_archive_student` 设为启动项目，在 `Student` 中编辑 [archive_value.cpp](src/student/archive_value.cpp)，其头文件和检查入口已归入同一项目。`Support/C2_archive_library` 保留真实静态库，`Reference/C2_archive_reference` 保留消费者，用于观察归档成员按需抽取；学生入口的合并不改变这个实验边界。学生项目中的 `Experiments/negative` 源码仅供浏览。

除明确标注的独立工具步骤外，下文命令也从 `exercises` 目录执行，使用已构建的 `vs-study`。

## Part 1：静态库正常消费者

`C2_archive_library` 包含两个成员：

- `used_member.cpp` 定义 `archive_value()`，消费者会调用。
- `unused_member.cpp` 定义 `unused_member_value()`，内部调用未定义的 `missing_unused_dependency()`。

`C2_archive_reference` 只调用 `archive_value()`：

```powershell
cmake --build --preset vs-study --target C2_archive_reference
ctest --preset vs-study -R '^C2_archive_reference$'
```

期望通过。

**解析：** 静态库不是把所有成员都无条件塞进最终程序。链接器按当前未解析符号抽取需要的 object。消费者只需要 `archive_value()` 时，`used_member` 被抽取，`unused_member` 不参与最终链接，它内部的缺失定义不会触发。

## Part 2：观察 object 与 archive 符号

运行：

```powershell
ctest --preset vs-study -R '^C2_archive_symbols$'
```

Windows/MSVC 的符号工具命令示意（将文件名替换为实际 object/lib 路径；可从构建输出及 `build/vs-study/C2_archive/evidence/symbols.txt` 定位）：

```powershell
dumpbin /symbols used_member.obj
dumpbin /linkermember:1 C2_archive_library.lib
```

ELF/GNU 路径保留为：

```bash
nm -C used_member.o
nm -C libC2_archive_library.a
ar t libC2_archive_library.a
```

**解析：** object 符号表证明 `used_member` 定义了 `archive_value`。archive linker member 输出证明静态库索引记录了这个符号。名字可能被 C++ ABI 修饰；MSVC COFF 常见 `?archive_value@@YAHXZ`。不要把源码函数名、decorated name 和调试器显示名混成同一个层级。

## Part 3：缺失定义负例

`negative/missing_definition` 只有声明和调用，没有定义 `missing_archive_value()`。测试先构建 object target，确认编译通过；再构建 link target，期望链接失败。

MSVC 期望诊断包含目标符号和 `LNK2019`/`LNK2001`/`unresolved external symbol`；GNU 期望 `undefined reference` 与目标符号。

**解析：** 声明足够让编译器生成调用代码；定义缺失要到链接阶段才暴露。configure 失败、编译失败、timeout 都不能算本负例通过。

## Part 4：直接链接所有 object 的负例

`negative/direct_objects` 把 `used_member.cpp` 和 `unused_member.cpp` 作为 object target 直接链接进 executable。这样 `unused_member` 也参与链接，其内部缺失的 `missing_unused_dependency()` 会触发链接失败。

**解析：** 这与 Part 1 正好形成对照：同样的源码成员，放进静态库且不被需求抽取时不会触发；直接链接 object 时会触发。这个实验说明“静态库正常链接”不等于“库里每个成员都能独立链接完整”。

## Part 5：链接顺序边界

GNU/ELF 常见左到右扫描静态库，库顺序可能影响解析；MSVC/COFF 的 `link.exe` 行为不能简单套用这条规则。本练习 Windows 实测 COFF 的 object/lib 行为；ELF 命令保留但未验证。

**解析：** 平台链接器策略是工程事实，不是标准 C++ 语义。课程写结论时必须说明工具链和证据来源。

## Part 6：Student

`vs-study` 已注册学生检查；修改实现后重新构建并运行：

```powershell
cmake --build --preset vs-study --target C2_archive_student
ctest --preset vs-study -R '^C2_archive_student$'
```

starter 应失败。完成条件：学生提供 `archive_value()` 的唯一定义并返回 42。

**解析：** 检查器链接学生 object 并直接调用目标符号。返回 0、删除检查或借用 Reference 都不是完成作业。
