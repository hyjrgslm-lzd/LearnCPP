# 12：Lua 栈不是 C 数组

Lua C API 把 C 和 Lua 的边界压到一个显式栈上。C 函数看见的不是“一个参数对象”，而是一段会随 push/pop 改变的索引空间。主案例仍是二进制记录转换：输入可能是 `61 00 7a ff`，输出必须是 `41 00 5a ff`。这个例子故意包含 NUL，因为它能把“字符串指针”和“字节序列”分开。

## 正确基线

最小正确路径是：保护区外先 `lua_checkstack`，再用零 upvalue C function 和 lightuserdata 进入 `lua_pcall`。保护区内记录 `lua_gettop`，用 `lua_pushlstring` 按显式长度压入输入，用 `lua_absindex` 固定要保存的槽位，再用 `lua_tolstring` 和返回长度读回。转换只看 ASCII `a-z`，其他字节原样复制。退出前恢复原栈顶。

负索引像“从当前栈顶倒数”。如果保存 `-1` 后又继续 push，`-1` 指向的对象就变了。`lua_absindex` 把当前可接受索引变成不随栈顶移动的绝对索引；registry 和 upvalue 这种 pseudo-index 不是普通栈槽，不应混成同一类生命周期。

## 先观察截断

L11 的 bad 先把输入复制进 `std::string`，再传给 `lua_pushstring`。这不会越界，但 Lua 只能按 C 字符串看到第一个 NUL 之前的字节。checker 输入 `{'a', 0, 'z', 0xff, 'A'}`，预期 `{'A', 0, 'Z', 0xff, 'A'}`；bad 会稳定触发：

```text
check failed: explicit byte length
```

这个证据证明的是长度协议错误，不证明任意悬空指针都可检测。调用方仍必须保证 `input.data()` 在 API 调用期间有效。

## registry、upvalue 和模块入口

短期值放栈，长期引用放 registry。`luaL_ref` 返回整数 key，host 必须在最后一次使用后 `luaL_unref`。C closure 的 upvalue 适合保存小型上下文指针，例如 L12/Lua backend 中 trampoline 读取的 POD 参数；它不是所有权转移。扩展模块用 `luaopen_xxx` 返回模块表，embedding 则由 host 创建 state、注册 C 函数并决定加载哪些库。两者共享 API 机制，但 owner 相反：模块被 Lua 加载，embedding 由 C++ host 管 Lua state。

## 解析

Reference 用 `lua_pushlstring` 和 `lua_absindex`，good 用同一契约但不同循环写法；两者都在退出前恢复栈顶。bad 只错在 NUL 语义，不依赖崩溃或 UB。运行 L11 时必须显式 `-DC18_ENABLE_LUA=ON` 且找到 Lua 5.4.9；当前环境缺 Lua，所以本章代码保持 UNVERIFIED。

继续：[13：protected call 和错误边界](13-lua-protected-errors.md)。
