# 14：userdata 保存对象，不替你决定 ownership

Lua full userdata 提供一块由 Lua 对象拥有的原始内存。C++ 可以在里面放 handle、指针或小对象，但必须说明谁关闭真实资源。`__gc` 是最后防线，不是唯一关闭路径；显式 close 和 GC finalizer 经常都会到达。

## 正确基线

L13 在 protected entry 里把一个小的 native handle 放进 `lua_newuserdatauv` 返回的内存，给它设置 metatable 和 `__gc`。handle 内有 `closed` 标志。显式 close 先运行，随后强制 GC 两次。正确结果是 finalizer 至少被调用一次，但真实关闭计数仍为 1。

这个模型对应插件或文件句柄包装：Lua 对象管理可见 lifetime，C++/host 管真实资源的关闭协议。userdata 地址只在 userdata 活着时有效；若对象标记了 finalizer，地址至少在 finalizer 调用期间有效。

## 先观察重复释放

bad 没有共享关闭状态：显式 close 计一次，`__gc` 又计一次。checker 不需要真的 double free；它用计数模型拒绝：

```text
check failed: close must be idempotent
```

这个证据证明状态协议错误。真实资源可能表现为 double close、重复 unregister、二次归还池对象或跳回已卸载模块；课程用安全计数替代真实破坏。

## metatable 和 user value

metatable 放行为，userdata 放状态。Lua 5.4 的 `lua_newuserdatauv` 还可以给 userdata 带 user value，用来挂 Lua 侧 owner，避免 C++ 只保存裸指针。不要把 light userdata 当成拥有对象；它只是一个指针值，没有 finalizer 和大小信息。

## 解析

Reference 与 good 都独立实现 close-once；bad 是行为反例。若未来把插件 context 放进 userdata，必须同时持有模块 lease，让最终释放发生在代码仍可调用时。当前环境缺 Lua 5.4.9，本章实现未编译运行，保持 UNVERIFIED。

继续：[15：coroutine、thread 和预算](15-lua-coroutines-threads.md)。
