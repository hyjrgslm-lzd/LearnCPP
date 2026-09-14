# 10. Limited API：abi3 是约束出来的

Stable ABI 的目标是让一个扩展二进制跨多个 CPython 3.x 版本加载。它不是“这个 `.pyd` 今天在我机器上能 import”，而是三件事同时成立：

```text
源码只使用 Limited API 可见接口
编译时定义固定 floor，例如 Py_LIMITED_API=0x03080000
链接到稳定 ABI 导入库，例如 Windows python3.lib / python3.dll
```

本课的 floor 固定为 Python 3.8.10。`0x03080000` 的含义是“我承诺只使用 3.8 及以前进入 Limited API 的符号”。用 3.10 头编译、或者用 Full API 结构字段，再自报一个字符串，都不是 ABI 证明。

Windows 上的 DLL 关系要分清。普通扩展常链接 `python38.lib` 或 `python310.lib`，运行时直接依赖对应版本 DLL。abi3 扩展应链接 `python3.lib`，运行时经 `python3.dll` 进入 Stable ABI。L09 现在把这一步做成实际检查：配置时找到已有 `dumpbin.exe` 或 `objdump`，测试先读取 `.pyd` 的 PE imports，只允许出现 `python3.dll`。如果 imports 里是 `python38.dll` 或 `python310.dll`，checker 在加载扩展前直接拒绝。

L09 的最小模块只有两个函数：

```text
abi_tag() -> "limited-0x03080000"
transform(bytes) -> bytes
```

`abi_tag()` 只是描述性标签，不能证明 ABI。真正证据来自组合：构建参数使用 `Py_LIMITED_API=0x03080000`，链接到 `python3.lib`，PE imports 只有 `python3.dll`，同一二进制被 Python 3.8.10 和 3.10.11 分别导入，导入后实际 bytes 行为通过检查。`transform` 保留 C18 的 ASCII bytes 契约：`a-z` 转大写，`NUL` 和高位字节不变。

验证路径必须使用同一个二进制。先用 Python 3.8.10 导入 `c18_l09_reference.pyd`，再用 Python 3.10.11 导入同一路径。如果重新构建两个 `.pyd`，就没有证明跨版本。CTest 里的 `c18_l09_reference_py38` 和 `c18_l09_reference_py310` 正是这两个入口。每次导入后，checker 会测空 bytes、包含 `NUL/0xff` 的 bytes、`bytes(range(256))`、重复长输入，并确认非 bytes 参数抛 `TypeError`。

bad 版本代表实际混淆：它按普通 Full API 扩展构建并链接版本专属 `python38.lib`。在 Windows 上，这会让 `.pyd` 直接依赖版本专属 Python DLL。checker 的诊断是 `extension depends on a version-specific Python DLL`。这个 bad 只在 Windows 注册，因为它检查的是 PE imports；非 Windows 不用这个 Windows DLL 依赖反例冒充失败。

Limited API 的代价是不能直接访问许多 CPython 内部结构。第 7 章那种手写 `PyTypeObject` 字段的 Full API 代码就不能原样搬来。真实项目要么接受更窄 API，要么放弃 abi3，不能把 Full API、pybind11 或当前版本可导入冒充稳定 ABI。

本章停止在 Python 3.8/3.10 已验证边界。当前证据不证明所有 Stable ABI 函数都被覆盖，也不证明未来 Python 版本一定可用；它只证明本题这个 Limited API 扩展在固定环境、固定 floor、固定二进制下满足声明。
