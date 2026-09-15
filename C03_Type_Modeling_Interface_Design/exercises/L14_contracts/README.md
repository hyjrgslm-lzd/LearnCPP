# L14：输入校验、断言、契约与库 hardening

先读 `../../chapters/14-contracts-hardening-and-interface-boundaries.md`。本练习是观察型，不提供 Student。程序只验证 C++23 可运行安全模型，不声称本机支持 C++26 contracts。

Part 1：`parse_port()` 处理不可信文本，返回 `std::expected<int, std::string>`。这是输入校验，不是断言。

Part 2：已校验的端口进入内部模型函数。内部函数有显式前置条件检查，用安全返回值表达模型，不触发未定义行为。

Part 3：打印 `_MSVC_STL_HARDENING`、`__cpp_contracts`、`__cpp_lib_contracts`。这只是能力观察，不触发标准库前置条件违反。

Part 4：记录 pattern matching 仍是提案跟踪项。没有标准宏就不声明支持。

运行：

```powershell
cmake -S C03_Type_Modeling_Interface_Design/exercises/L14_contracts -B build/author-c-l14 -G "Visual Studio 18 2026" -A x64
cmake --build build/author-c-l14 --config Release
ctest --test-dir build/author-c-l14 -C Release --output-on-failure
```

解析：外部输入错误走错误通道；断言和 contracts 表达程序员责任；library hardening 是标准库实现策略。C++26/C++29 真实语法能力统一看 F01，不在本练习重复造 probe 框架。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L14_contracts_observation`，可直接设为启动项运行观察或专项入口。
没有 Student 编辑入口；运行/阅读：`contracts_observation.cpp`。这些入口保留在题目分组内，作为观察、探测或负例诊断。
构建主项目后运行 CTest 或 README 中列出的检查命令；负例/探测入口只读，用于观察诊断。
