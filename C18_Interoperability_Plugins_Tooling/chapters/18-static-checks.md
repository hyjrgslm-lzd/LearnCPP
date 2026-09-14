# 18. Static checks for the C18 ABI

静态检查器不能代替 ABI 设计。它只能把“明显不该进 C ABI 的形态”挡在提交前。C18 的 P1 ABI 已经冻结：唯一导出是 `c18_get_api`，长期状态是 `c18_context*`，调用面在 `c18_api` 函数表里。

最小坏例子：

```cpp
extern "C" std::string c18_bad_name();
extern "C" c18_status c18_bad_vector(std::vector<unsigned char>* bytes);
```

这两个声明在 C++ 里能解析，但都不该出现在 C ABI。`std::string` 牵涉实现布局、分配器和异常；`std::vector` 指针把 C++容器所有权暴露给外部。相反，`c18_context*` 是不完整类型指针，host 不能解引用，只能交回函数表。

## From type spelling to canonical type

检查不能只看拼写。`typedef c18_status (*c18_process_fn)(...)` 在字段处看起来只是 typedef 名，真正要检查的是 canonical type 里函数指针的返回值和参数。`size_t`、`uint32_t`、`uint8_t` 也可能经 typedef 出现；它们是允许的整数族。

检查器输出要指向 spelling location，因为用户改的是源文件里的拼写位置。诊断文本固定为：

```text
dangerous exported ABI type
```

## Exercise

L16 覆盖检查器规则；P2 的 `check` 子命令把同一规则接入真实 `ClangTool`。输入是 P1/C18 ABI 头和自有夹具，输出是按文件/行号列出的诊断。它不修改任何文件。

本题的 checker 入口在 ON 构建中运行：每个 variant 和 `l16_driver.cpp` 链成一个原生工具，driver 负责 Clang 解析，variant 负责 AST 规则。它的边界是教学 allowlist：能发现代表性 C++ 类型泄漏，不能证明所有 C ABI 兼容性。
