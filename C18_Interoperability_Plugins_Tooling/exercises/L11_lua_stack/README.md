# L11 Lua stack

只编辑 `student/solution.hpp`。目标是用 Lua 5.4.9 C API 把一段二进制字节压入 Lua，再按显式长度取回；不能靠 NUL 结尾。

## Part 1

保护区外先 `lua_checkstack`，再用零 upvalue C function 和 lightuserdata 进入 `lua_pcall`。保护区内用 `lua_pushlstring`，保存 `lua_absindex` 后再读值。负索引随栈顶移动，长期保存时必须转成绝对索引。

## Part 2

用 `lua_tolstring(..., &len)` 取得长度。字符串尾部有一个 C NUL 方便互操作，但字符串体内仍可包含 NUL。

## Part 3

进入函数时记录 `lua_gettop`，退出前恢复。checker 会用含 NUL 与 `0xff` 的输入拒绝 `lua_pushstring` 式截断。

```sh
cmake -S C18_Interoperability_Plugins_Tooling/exercises/L11_lua_stack -B C18_Interoperability_Plugins_Tooling/build/l11-lua -DC18_ENABLE_LUA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build C18_Interoperability_Plugins_Tooling/build/l11-lua --config Release
ctest --test-dir C18_Interoperability_Plugins_Tooling/build/l11-lua -C Release --output-on-failure
```

当前机器未安装 Lua 5.4.9 时，显式 `C18_ENABLE_LUA=ON` 应在配置阶段失败；这不是通过。
