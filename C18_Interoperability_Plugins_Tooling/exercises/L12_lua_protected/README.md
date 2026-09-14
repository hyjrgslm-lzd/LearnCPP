# L12 Lua protected errors

只编辑 `student/solution.hpp`。目标是把可失败 C++ helper 放到受保护 Lua C trampoline 后面：helper 捕获异常、清理本层资源、返回 POD 错误；trampoline 再把错误推入 Lua 栈并调用 `lua_error`。

## Part 1

保护区外只做 `luaL_newstate`、`lua_checkstack`、零 upvalue C function 和 lightuserdata。用 `lua_pcall` 或 `lua_pcallk` 调 C function。`lua_error` 使用 Lua 的非局部跳转，不能穿过带非平凡析构的 C++ 栈帧。

## Part 2

C++ helper 不直接碰 Lua 栈；它只返回 `{code, message}`。错误字符串由 C trampoline 入栈，然后 `lua_error`。

## Part 3

`pcall` 返回后再读错误对象并清空栈。checker 要求成功和失败两条路径都走真实 Lua entry、清理计数正确，bad 会在 `pcall` 返回后吞掉真实 Lua 错误。

```sh
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L12_lua_protected -B C18_Interoperability_Plugins_Tooling/build/l12-lua -DC18_ENABLE_LUA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build C18_Interoperability_Plugins_Tooling/build/l12-lua --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/l12-lua -C Release --output-on-failure
```

缺 Lua 5.4.9 时本题应配置失败并保持 UNVERIFIED。
