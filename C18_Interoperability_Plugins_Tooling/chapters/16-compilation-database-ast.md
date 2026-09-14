# 16. Compilation database and AST

C18 前半段一直在防止 ABI 说谎：头文件里看起来只是几个函数指针，真实调用时却牵涉调用约定、结构体大小、opaque handle、异常边界和生命周期。Clang 工具解决的是另一个说谎点：源码文本看起来像一次调用，编译器看到的可能是宏展开、模板实例化、隐式构造、typedef、重载解析之后的节点。

所以第一步不是写检查器，而是拿到和项目构建一致的编译语境。`compile_commands.json` 记录每个翻译单元的工作目录、源码路径和编译参数。P2 要用 WSL 中的 Ninja 生成它。没有它，工具可能用错 include path、宏、语言标准或目标平台；这时 AST 能生成，也不一定是项目真实 AST。

## Baseline

L15 只做观察。下面源码很小：

```cpp
struct Api {
  int (*process)(int);
};

int run(Api& api) {
  return api.process(4);
}
```

用 Clang 18.1.3 解析后，`Api` 在 C++ AST 中是 `CXXRecordDecl`，函数体里的调用是 `CallExpr`，成员访问是 `MemberExpr`。L15 使用 JSON AST，不靠文本共现；checker 找到 `Api` 的 `FieldDecl process`，再要求某个 `CallExpr` 下面的 `MemberExpr process` 的 `referencedMemberDecl` 指向这个字段，接收者类型是 `Api` 或 `Api*`。

## Why text is not enough

如果用字符串找 `process(`，三个场景立刻出错：

1. `api.process(4)` 和 `api->process(4)` 文本不同，语义类似。
2. `#define CALL(x) api.process(x)` 的调用位置来自宏展开，改写文本位置会错。
3. `typedef c18_status (*c18_process_fn)(...)` 之后，真正危险的是函数指针目标类型，不是字段名字本身。

AST 的价值是把“文本长什么样”变成“编译器解析到了什么”。但 AST 也不是魔法。模板实例化会生成额外节点；隐式构造和隐式转换可能出现在 dump 中；SourceLocation 可能指向 spelling 位置，也可能指向 expansion 位置。后面几章的工具都先保守拒绝不能安全定位的写入。

## Exercise

做 `exercises/L15_ast`。学生只编辑 `student/solution.py`，返回一段能被 Clang 解析的源码。bad 里有 `Other::process` 调用，但没有 `Api::process` 调用；它用于证明 checker 看的是 AST 节点关系，不是几个词同时出现。

如果本机没有可用 Clang，L15 可以 SKIP。语法错误不是能力缺失，必须 FAIL。
