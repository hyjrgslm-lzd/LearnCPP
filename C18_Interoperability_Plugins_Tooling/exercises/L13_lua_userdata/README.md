# L13 Lua userdata GC

只编辑 `student/solution.hpp`。目标是用 full userdata 保存一个 C++ 对象占位，并让显式 close 与 `__gc` finalizer 共享同一个关闭状态。

## Part 1

保护区外先检查栈空间；保护区内用 `lua_newuserdatauv` 创建 full userdata，再设置 metatable。userdata 内存由 Lua 对象拥有，指针只在 userdata 活着时有效。

## Part 2

显式 close 和 `__gc` 都可能到达。二者必须共用 `closed` 标志；重复 GC 或先 close 后 GC 不能重复释放原生资源。

## Part 3

checker 先显式 close，再强制收集两次。bad 会在 close 和 finalizer 两边各计一次释放，被稳定拒绝。

```sh
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L13_lua_userdata -B C18_Interoperability_Plugins_Tooling/build/l13-lua -DC18_ENABLE_LUA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build C18_Interoperability_Plugins_Tooling/build/l13-lua --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/l13-lua -C Release --output-on-failure
```

缺 Lua 5.4.9 时本题保持 UNVERIFIED。
