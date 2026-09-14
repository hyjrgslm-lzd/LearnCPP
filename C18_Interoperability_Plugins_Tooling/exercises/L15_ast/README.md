# L15 AST observation

本题只观察真实 Clang AST，不写 LibTooling 工具。`checks.py` 会把 `solution.py` 返回的源码喂给 `clang++ -Xclang -ast-dump -fsyntax-only -`，再检查 dump 里是否真的出现结构体、函数、调用表达式、成员表达式和隐式声明节点。

学生只编辑 `student/solution.py` 的 `source()`。目标是写出一个小例子：`Api` 有函数指针字段 `process`，某个函数通过 `api.process(...)` 或 `api->process(...)` 调用它。源码必须能被 Clang 18.1.3 解析。

Reference 和 good 是两份独立源码。bad 能解析，但没有调用 `Api::process`，checker 必须拒绝它：

```text
check failed: AST dump did not contain a resolved call through Api::process
```

这个检查证明“AST观察入口可用”。它不证明 L16-L18 或 P2 的 LibTooling 工具已经编译、链接或运行。
