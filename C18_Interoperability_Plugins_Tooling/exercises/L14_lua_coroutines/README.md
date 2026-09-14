# L14 Lua coroutines

只编辑 `student/solution.hpp`。目标是用 `lua_newthread`、`lua_resume` 和 `lua_yield` 理解 coroutine 的独立栈、registry 存活和 owner-thread 调度边界。

## Part 1

setup 先进入 protected entry；在里面创建 thread、注册 C continuation 函数、加载 chunk。`lua_newthread` 创建的 thread 会被 GC，所以 host 要把它放进 registry，直到最后一次 resume 后再 unref。

## Part 2

`lua_resume` 返回 `LUA_YIELD` 表示 coroutine 暂停，返回 `LUA_OK` 表示结束。本题先用 Lua 脚本 yield，再用 C function `lua_yieldk` yield，第三次 resume 进入 continuation 返回最终值。

## Part 3

Lua state 是 owner-thread 资源。其他线程只投递请求到 owner 队列；不能直接 resume。checker 核对真实 worker thread id、owner drain 后的 resume thread id、yield 序列和 continuation。bad 真实调用 affinity checked hook，在碰 Lua 前被拒绝。

```sh
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L14_lua_coroutines -B C18_Interoperability_Plugins_Tooling/build/l14-lua -DC18_ENABLE_LUA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build C18_Interoperability_Plugins_Tooling/build/l14-lua --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/l14-lua -C Release --output-on-failure
```

缺 Lua 5.4.9 时本题保持 UNVERIFIED。
