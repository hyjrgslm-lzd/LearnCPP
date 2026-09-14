# 15：coroutine 暂停的是 Lua thread，不是 host 线程

Lua 的 `lua_State*` 可以表示主 state，也可以表示 coroutine thread。`lua_newthread` 创建的新 thread 共享全局环境，但有独立 Lua 栈；它本身仍是 Lua GC 对象。host 如果要长期 resume 它，就必须把 thread 放进 registry。

## 正确基线

L14 的 setup 也先进入 protected entry：注册 C continuation 函数、创建 coroutine、放入 registry、加载 chunk 都在 `lua_pcall` 内完成。chunk 先 Lua 脚本 yield 1，再调用 C function；这个 C function 用 `lua_yieldk` yield 2，第三次 resume 进入 continuation 并返回 3。host 第一次 `lua_resume` 得到 `LUA_YIELD` 和 1；第二次得到 `LUA_YIELD` 和 2；第三次得到 `LUA_OK` 和 3。每次读完结果都 pop。最后 `luaL_unref`，再 close state。

yield 穿过 C API 有严格例外：`lua_yieldk`、`lua_callk`、`lua_pcallk` 通过 continuation 继续执行。普通 C 函数不能在带非平凡 C++ 析构的帧外侧等待 Lua yield 后“回来接着跑”。

## owner-thread 队列

Lua state 没有变成可从任意 C++ 线程调用的对象。课程约定一个 owner 线程拥有 state；其他线程只能投递请求到线程安全队列，由 owner drain 队列后执行 `lua_resume`。L14 checker 检查真实 worker thread id、最后一次 resume thread id、Lua yield 结果和 continuation 标志。bad 的 worker 真实调用 affinity checked resume hook；hook 在碰 Lua 前拒绝，并记录 foreign thread id，所以不会制造真实跨线程数据竞争：

```text
check failed: owner queue
```

这证明的是 host 协议，不是 Lua VM 的安全沙箱能力。

## 资源预算和 hook

`lua_sethook` 可以设置 instruction count hook，适合做教学里的“运行步数预算”。它不能保证安全沙箱：hook 运行时会禁用其他 hook；C 函数、内存、I/O、宿主暴露的库函数都可能绕开你以为的边界。内存预算应由 allocator 或外层资源策略配合；instruction hook 只能说明“在这个执行路径上触发过计数中断”。

## 解析

Reference 保存 coroutine registry ref，worker 投递 resume 任务，owner drain 后触碰 Lua。good 独立实现相同序列。bad 真实启动 worker 并调用 affinity hook，hook 在触碰 Lua 前拒绝，随后 owner 路径完成 Lua 序列以证明 checker 拒绝的是线程归属错误。当前环境缺 Lua 5.4.9，本章实现未编译运行，保持 UNVERIFIED。
