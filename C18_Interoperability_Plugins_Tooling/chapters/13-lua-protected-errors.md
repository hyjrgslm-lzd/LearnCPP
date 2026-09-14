# 13：protected call 是边界，不是 C++ catch

Lua 5.4 的错误和 yield 都可能使用 C 的非局部跳转。Lua 以 C 编译时，这条跳转不会按 C++ 异常规则展开沿途对象；因此不能把 `try/catch` 放在外层，期待捕获 `lua_error`，也不能让 `lua_error` 跨过带非平凡析构的 C++ 帧。

## 正确基线

边界分两层。保护区外只做可控 bootstrap：`luaL_newstate` 失败返回空指针，`lua_checkstack` 失败返回状态；随后压入零 upvalue C function 和 lightuserdata，进入 `lua_pcall`。C++ helper 做普通 C++ 工作，捕获自己的异常，完成本层清理，然后返回 POD：错误码和静态消息。Lua C trampoline 读取 POD，必要时把错误对象压入 Lua 栈，再调用 `lua_error`。外层用 `lua_pcall` 或 `lua_pcallk` 接住错误码，读错误对象，最后清空栈。

这不是为了把代码写复杂；这是为了让每种控制流只跨它能安全跨过的帧。C++ 异常只在 C++ helper 内展开。Lua longjmp 只在 protected Lua 调用内部跳转。

## 先观察吞错

L12 的 bad 也真实进入 Lua：它在 protected C entry 中触发 `lua_error`，`lua_pcall` 返回后却把状态改回成功。checker 运行成功和失败两条路径，失败路径要求：外层确实走 protected call，bootstrap 检查通过，helper 清理计数为 1，状态不是 `LUA_OK`，错误消息包含 helper。bad 会触发：

```text
check failed: protected trampoline error
```

这个反例不制造真实 UB。它证明“错误必须进入 Lua protected boundary”，不是证明某个未受保护崩溃每次可复现。

## 参数入栈和清理也算边界

容易漏掉的是参数准备阶段。`lua_pushlstring`、`lua_newuserdatauv`、`luaL_loadbufferx`、`luaL_openlibs` 等 API 可能分配内存并触发 Lua 错误路径。若外层 C++ 帧已经持有需要析构的对象，就不要让这些调用的 longjmp 越过它。课程 backend 把真实 Lua 脚本调用集中到 `lua_boundary.c`：C++ 只分配 vector，调用 C 函数，收到 POD 结果后再抛 `std::runtime_error` 给 workbench。C 文件在 `lua_pcall` 内 open libs、建 closure、压字符串和运行脚本，失败消息复制进有界 POD。

## 解析

Reference 的 trampoline 调 C++ helper，helper catch 后返回 POD，trampoline 再 `lua_error`。good 独立使用 `lua_pcallk`，表达同一边界。bad 正常退出 1 被 checker 拒绝。当前环境缺 Lua 5.4.9，本章实现未编译运行，保持 UNVERIFIED。

继续：[14：userdata、metatable 和 GC](14-lua-userdata-gc.md)。
