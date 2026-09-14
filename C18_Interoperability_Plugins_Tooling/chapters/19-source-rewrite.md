# 19. Source rewrite

重构工具的危险不是“找不到字符串”，而是“找到了不能安全改的位置”。Clang 的 `SourceManager` 同时维护 spelling location 和 expansion location。宏里的 token 可能拼写在宏定义处，却展开在调用处；模板里的节点可能来自模板定义，也可能来自实例化上下文。

C18 只迁移一个受控符号：

```text
c18_process_old -> c18_process
```

迁移范围只有普通自有文件中的声明和已解析引用。宏、模板、系统头全部拒绝。拒绝比猜更便宜，也更可靠。

## Replacement flow

P2 `rewrite` 子命令的形状是：

1. `MatchFinder` 找 `namedDecl(hasName(old_symbol))` 和 `declRefExpr(to(namedDecl(hasName(old_symbol))))`。
2. callback 取 `SourceLocation`。
3. `SourceManager` 检查 `isMacroID()`、`isMacroArgExpansion()`、`isMacroBodyExpansion()`、`isInSystemHeader()`。
4. 对模板声明或模板上下文拒绝。
5. 用 `tooling::Replacement` 记录替换。
6. 用 `Replacements` 汇总并检查 conflict。
7. 默认打印 patch；指定 `out-dir` 时写副本。

任何情况下都不原地改输入文件。跨 TU 收集到同一文件时，按文件汇总去重；冲突就是失败，不用最后一个覆盖前一个。

## Exercise

做 `exercises/L17_rewrite`。bad 是字符串替换；它会改到宏、模板和注释。正确答案必须先让 Clang 解析输入，普通源只写输出副本，宏/模板返回拒绝诊断，重复运行迁移后的输出应保持成功且无额外替换。
