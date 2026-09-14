# Clang 18.1.3 source reading

版本固定为 `llvmorg-18.1.3`。当前课程只使用这个版本讨论接口入口，不用 rolling main 推断 API。

## Tool entry

`clang/include/clang/Tooling/Tooling.h` 是 standalone tool 的入口。关键事实：

- `ClangTool` 用 `CompilationDatabase` 和 source path 列表构造。
- `ClangTool::run(ToolAction*)` 对每个翻译单元运行 action；源码注释说明它返回 `0` 表示成功，`1` 表示错误，`2` 表示有文件因缺 compile command 被跳过。
- `FrontendActionFactory::create()` 为每个 TU 生成新的 `FrontendAction`。
- `newFrontendActionFactory()` 可把 `ASTFrontendAction` 或 consumer factory 接到 tool 上。

课程中的 P2 主线对应：

```text
CommonOptionsParser -> getCompilations/getSourcePathList -> ClangTool -> FrontendActionFactory
```

## Compilation database

`clang/include/clang/Tooling/CompilationDatabase.h` 定义 `CompilationDatabase` 和 `CompileCommand`。`loadFromDirectory()` 查构建目录里的 `compile_commands.json`；`autoDetectFromSource()` 从源码路径向上找构建数据库。P2 不猜 include path，而是要求 Ninja/CMake 生成数据库。

`clang/lib/Tooling/JSONCompilationDatabase.cpp` 是 JSON 数据库实现入口。它注册 `compile_commands.json` 插件，解析每条 compile command，再按源文件返回命令。

## AST traversal

`clang/include/clang/AST/RecursiveASTVisitor.h` 定义 RAV。默认不访问模板实例化，也不访问隐式代码：

```cpp
bool shouldVisitTemplateInstantiations() const { return false; }
bool shouldVisitImplicitCode() const { return false; }
```

C18 ABI检查要看模板实例化带来的危险类型时可以打开前者；L15观察隐式节点时只读 AST dump，不把隐式节点当用户可编辑位置。

## Matchers

`clang/include/clang/ASTMatchers/ASTMatchFinder.h` 定义 `MatchFinder`、`MatchCallback` 和 `MatchResult`。P2 rewrite 用 matcher 绑定声明和引用：

```cpp
namedDecl(hasName(old_symbol)).bind("decl")
declRefExpr(to(namedDecl(hasName(old_symbol)))).bind("ref")
```

callback 中只处理绑定节点，不从文本再猜一次。

## Source locations and rewrites

`clang/include/clang/Basic/SourceManager.h` 提供 `getSpellingLoc()`、`getExpansionLoc()`、`isMacroArgExpansion()`、`isMacroBodyExpansion()`、`isWrittenInMainFile()`、`isInSystemHeader()`。P2 的默认策略保守：宏和系统头拒绝；模板也拒绝。

`clang/include/clang/Tooling/Core/Replacement.h` 定义 `Replacement`、`Replacements` 和 `applyAllReplacements()`。P2 先收集 replacement，再按文件汇总。冲突返回错误；不能让后一个替换静默覆盖前一个。

## Current local limit

WSL `LearnCPP-C08-Ubuntu-24.04` 有 `/usr/lib/llvm-18/bin/clang++` 18.1.3，可运行 AST dump。当前缺 `clang/Tooling/Tooling.h`、`clang/ASTMatchers/ASTMatchFinder.h`、`clang/Rewrite/Core/Rewriter.h` 和可链接 `libclang-cpp.so` symlink，所以 LibTooling 主体保持 `UNVERIFIED`。

因此本课默认验证分两层：L15 用固定 Clang 18.1.3 的 JSON AST 做真实观察；`C18_ENABLE_CLANG_TOOLING=ON` 只在上述开发头和库全部存在时进入原生 LibTooling 编译与 CTest。L16-L18 不用 JSON AST 替代 LibTooling。
