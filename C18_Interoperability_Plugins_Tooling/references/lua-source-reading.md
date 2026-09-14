# Lua 5.4.9 source reading guide

版本固定：Lua 5.4.9。官方源码页 `lua.h` 标明 `LUA_VERSION_RELEASE "9"` 和 `LUA_VERSION_RELEASE_NUM (LUA_VERSION_NUM * 100 + 9)`。本导读只读官方源码和手册；当前机器缺 Lua 5.4.9 开发包，课程 Lua 代码未编译运行，标记 UNVERIFIED。

## C API 入口

- `lua.h`：公共 API 声明。重点看 `lua_absindex`、`lua_pushlstring`、`lua_tolstring`、`lua_newuserdatauv`、`lua_pcallk`、`lua_resume`、`lua_yieldk`、`lua_sethook`。
- `lapi.c`：栈访问、push/get/set、userdata 创建等 API 实现入口。`lua_newuserdatauv` 创建 full userdata，压栈并返回 userdata 内存地址。
- `lauxlib.c`：辅助库。`luaL_error` 最终调用 `lua_error`；`luaL_traceback` 说明错误对象和栈回溯需要在 protected call 边界内准备。

## Protected call、yield 和退出路径

- `ldo.c`：`luaD_rawrunprotected`、`luaD_pcall`、`lua_resume`、`lua_yieldk` 是本课错误/yield 边界的主入口。Lua 以 C 编译时，错误和 yield 依赖 C longjmp 模型；C++ 不能用外层 catch 捕获它。
- `lstate.c`：`lua_newstate`、`lua_close`、thread 创建/关闭路径。host 拥有 state 时，必须清楚 close 前是否还有 registry ref、userdata finalizer 或 coroutine。
- `ldebug.c`：debug hook 设置与触发。instruction hook 可以做预算入口，但不是安全沙箱。
- `lgc.c`：GC 标记、清扫、finalizer 调度。userdata `__gc` 到达时，原生资源必须支持 idempotent close。

## 和课程实现的对应

- L11 对应 `lua_absindex`、`lua_pushlstring`、`lua_tolstring`：证明显式长度和栈平衡。
- L12 对应 `lua_pcallk`、`lua_error`、`ldo.c` protected boundary：C++ helper 返回 POD，C trampoline 才抛 Lua 错误。
- L13 对应 `lua_newuserdatauv`、metatable、`__gc`：显式 close 与 GC finalizer 共用关闭状态。
- L14 对应 `lua_newthread`、`lua_resume`、`lua_yieldk`、`lua_sethook`：coroutine registry 存活、owner-thread resume 和预算边界。
- `exercises/backends/lua_boundary.c` 是 P1 的真实 Lua C API backend：一次调用创建 state，保护区外只做 stack bootstrap，`lua_pcall` 内打开库、建 closure、压入显式长度字符串、执行 Lua 脚本、复制有界错误，最后关闭 state。C++ wrapper 只把 POD 错误转成 `std::runtime_error`。

## 手册边界

Lua 5.4 手册说明：`lua_tolstring` 返回长度且字符串体内可含 NUL；`lua_pcall` 捕获错误并返回状态码；yield 跨 C API 只有 continuation 相关函数可行；hook 不能承担完整沙箱职责。本课的 checker 只验证这些边界在课程代码中的代表性使用，不声称覆盖 Lua VM 所有状态。
