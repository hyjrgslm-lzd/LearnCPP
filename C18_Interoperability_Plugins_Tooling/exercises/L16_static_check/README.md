# L16 static ABI checks

本题把 L15 的观察变成检查器。学生编辑 `student/solution.cpp`，实现 `c18_tooling::inspect_abi()`。ON 构建时，`P2_abi_tool/src/l16_driver.cpp` 创建 `ClangTool -> ASTFrontendAction -> ASTConsumer`，在 `HandleTranslationUnit()` 调用所选 variant。

1. 找 `extern "C"` 且名字以 `c18_` 开头的导出函数。
2. 找 `c18_api` 函数表字段。
3. 检查返回值、参数、字段类型。
4. 允许 `c18_context*` opaque handle、整数、`void*`、字节指针、函数指针。
5. 拒绝 `std::string`、`std::vector`、C++ reference、class 直接暴露，并输出 `dangerous exported ABI type`。

Reference 与 good 独立：二者各自实现 `RecursiveASTVisitor`。bad 只看函数定义，会漏掉 ABI 头中的声明，checker 必须拒绝。当前环境缺 LibTooling dev 组件，默认 OFF 不注册本题算法 PASS；显式 ON 后若依赖齐全才编译并执行 variants。
