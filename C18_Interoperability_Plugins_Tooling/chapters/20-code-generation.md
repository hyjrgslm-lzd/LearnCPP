# 20. Code generation

代码生成很容易滑成“大而全 binding generator”。C18 不做这个。P2 `generate` 只输出两类东西：

1. ABI 导出清单：例如 `c18_get_api` 和 `c18_api` 字段。
2. 可编译消费者：一个 C 文件和一个 C++ 文件，用编译期断言检查版本、表大小和关键字段形态。

这和 Python/Lua binding 是两条线。binding 需要对象生命周期、错误转换、线程/GIL或Lua state所有权；P2 只证明“这个ABI头还能被C和C++消费者按约定包含和检查”。

## Why generated consumers matter

清单是给人看的，消费者源码是给编译器看的。比如：

```c
#include "c18/abi.h"
_Static_assert(C18_ABI_VERSION == 1u, "version");
_Static_assert(sizeof(c18_api) >= sizeof(void*) * 4, "api table has function slots");
```

C++侧用 `static_assert`。两份消费者都应该进入 build 副本目录，再由编译命令验证。当前环境缺 LibTooling 主体构建能力，所以 `c18_tool generate` 和 L18 variants 标 `UNVERIFIED`；生成、落文件和消费者编译的 CTest 入口已经注册在 ON 分支。

## Exercise

做 `exercises/L18_codegen`。学生输出 export manifest 和 C/C++ contract consumer。bad 把任务扩大成通用 binding generator，checker 必须拒绝。

P2 完整运行后，验收不是“打印了几行文本”，而是：

- `generate` 产生 manifest。
- 生成的 C consumer 能以 C11 编译。
- 生成的 C++ consumer 能以 C++23 编译。
- 生成物只落在 build/output 目录。
