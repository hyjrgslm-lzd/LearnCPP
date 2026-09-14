# P2 ABI tool

`c18-tool` 是本课的 Clang 工程项目。它有三个子命令：

- `check`：读取 P1/C18 的 ABI 头，拒绝 `extern "C"` 导出中的 C++ reference、STL、class 等危险签名；opaque `c18_context*`、POD整数、显式指针/长度、函数表允许。
- `rewrite`：只把受控旧符号 `c18_process_old` 迁移到 `c18_process` 的声明和已解析引用。宏展开、模板实例化、系统头、替换冲突全部拒绝。
- `generate`：输出 ABI 导出清单和可编译 C/C++ 契约检查代码；它不是通用 binding generator。

默认构建不编译 LibTooling 主体，也不注册 P2 行为 PASS。显式打开 `-DC18_ENABLE_CLANG_TOOLING=ON` 时，使用 Ninja 并导出 compilation database：

```powershell
wsl -d LearnCPP-C08-Ubuntu-24.04 -- cmake -G Ninja -S /mnt/f/CPPTrain/LearnCPP/C18_Interoperability_Plugins_Tooling/exercises/P2_abi_tool -B /mnt/f/CPPTrain/LearnCPP/C18_Interoperability_Plugins_Tooling/build/p2-clang -DC18_ENABLE_CLANG_TOOLING=ON
```

`ClangSupport.cmake` 会要求 LLVM/Clang 18.1.3、Tooling/ASTMatchers/Rewriter 头和可链接 `libclang-cpp.so`。当前 WSL 环境缺这些开发组件，所以主体保持 `UNVERIFIED`。

工具永远不原地改输入。`rewrite` 默认打印 patch 计划；给 `--out-dir` 时只写新副本。输出目录必须是新目录或不含目标文件；如果输出路径等于输入、目标文件已存在、符号链接已占位、或多个输入会落到同一个 `filename`，工具拒绝执行。`generate` 同样只用 create-new 语义写 `exports.txt`、`contract_check.c`、`contract_check.cpp`，不会覆盖已有文件。

`verify_rewrite.py` 与 `verify_codegen.py` 只是 CTest driver：它们启动原生二进制、检查输出/哈希/生成物编译，不实现 AST 算法。
