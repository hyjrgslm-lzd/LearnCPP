# L18 code generation

本题的生成只服务 ABI 契约，不做通用 binding generator。学生编辑 `student/solution.cpp`，实现 `inspect_manifest()` 和 `emit_contract()`。

输出需要三块：

1. `exports`：至少列出 `c18_get_api` 和 `c18_api`。
2. C消费者：`#include "c18/abi.h"`，用 `_Static_assert` 检查版本、表大小和关键字段。
3. C++消费者：同样 include ABI 头，用 `static_assert` 做编译期契约检查。

bad 把目标扩大成 binding generator，只写清单名、不写 C/C++ contract consumer，checker 必须拒绝。ON 构建时，`l18_driver.cpp` 先通过 Clang AST 收集 ABI 形状，再调用所选 variant 生成文件；CTest driver 用真实 C/C++ 编译器做 `-fsyntax-only` 检查。默认 OFF 不注册算法 PASS。

`out-dir` 必须是新目录或不含目标文件；生成器用 create-new 语义写文件，遇到已有输出或与输入别名相同的路径会失败。
