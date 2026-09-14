# 17. AST matchers and RecursiveASTVisitor

写工具有两条常用路。`RecursiveASTVisitor` 像手工遍历：从翻译单元往下走，看到 `FunctionDecl`、`FieldDecl`、`DeclRefExpr` 时自己判断。AST Matchers 像声明式过滤：写出“我要某类节点，且它满足这些关系”，然后在 callback 中处理绑定节点。

C18 的 ABI 检查适合 RAV。规则需要沿类型递归判断：导出函数的返回值、每个参数、`c18_api` 字段的函数指针目标，都要区分安全 C 形态和危险 C++ 形态。当前 ABI 头没有自定义 annotate；入口就是现有事实：`extern "C"`、`c18_*` 前缀、`c18_api/c18_host_api/c18_context` 和函数指针 typedef。

## Conservative rule

L16 采用保守筛选：

- 允许：整数、`void*`、`uint8_t*`、`size_t*`、函数指针、`c18_context*` opaque handle、`c18_api`/`c18_host_api` 这样的 C-compatible record。
- 拒绝：C++ reference、`std::string`、`std::vector`、模板特化、直接暴露的非ABI class。

这个规则只证明“发现代表性危险签名”。它不证明整个插件生命周期安全，也不证明不同编译器/平台的 ABI 完全一致。生命周期仍由前面章节和 P1 运行验证覆盖。

## FrontendAction shape

固定 LLVM 18.1.3 中，`clang::tooling::ClangTool` 接收 `CompilationDatabase` 和源码列表；`FrontendActionFactory::create()` 为每个翻译单元创建 action；`ASTFrontendAction::CreateASTConsumer()` 返回 consumer；consumer 在 `HandleTranslationUnit()` 中遍历 AST。

P2 的 `check` 子命令就是这个形状：

```text
CommonOptionsParser -> CompilationDatabase -> ClangTool
ClangTool -> CheckAction -> CheckConsumer -> RecursiveASTVisitor
```

## Exercise

做 `exercises/L16_static_check`。学生在 `solution.cpp` 里补保守 ABI 检查逻辑；它必须通过 `RecursiveASTVisitor` 消费真实 AST，不能只返回固定文本。P2 的 `src/c18_tool.cpp` 是同一规则的集成工具：`VisitFunctionDecl` 必须处理 `extern "C"` 声明，不只处理定义；`VisitFieldDecl` 处理 `c18_api` 函数表字段。

当前环境缺 LibTooling dev 组件，默认 OFF 不跑 L16-L18 算法测试。显式打开 `C18_ENABLE_CLANG_TOOLING=ON` 时，私有 `P2_abi_tool/ClangSupport.cmake` 会先检查真实开发组件，缺头或库就是 configure FAIL。
